/**
 * protocol.c - WebP2P HTTP protocol implementation.
 * Summary: Builds authenticated index requests and parses their responses.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#define _POSIX_C_SOURCE 200809L
#include "protocol.h"

#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WEBP2P_REGISTER_PREFIX "REDP2P-WEB-REGISTER"
#define WEBP2P_ADMISSION_PREFIX "REDP2P-ADMISSION-v1"

typedef struct {
    char nonce[65];
    char mac[65];
    uint64_t issued_at;
    uint64_t expires_at;
    unsigned bits;
} webp2p_challenge_t;

typedef struct {
    char *data;
    size_t size;
} webp2p_http_buffer_t;

/**
 * Formats an allocated string from variadic arguments.
 * @param format Printf-style format string.
 * @return Allocated formatted string, or NULL on failure.
 */
static char *webp2p_format(const char *format, ...)
{
    va_list args;
    va_list copy;
    char *out;
    int size;

    va_start(args, format);
    va_copy(copy, args);
    size = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (size < 0) {
        va_end(args);
        return NULL;
    }
    out = (char *)malloc((size_t)size + 1);
    if (!out) {
        va_end(args);
        return NULL;
    }
    (void)vsnprintf(out, (size_t)size + 1, format, args);
    va_end(args);
    return out;
}

/**
 * Skips leading whitespace in text.
 * @param text Input text.
 * @return First non-whitespace character, or the terminating null character.
 */
static const char *webp2p_skip_space(const char *text)
{
    while (text && *text && isspace((unsigned char)*text)) text++;
    return text;
}

/**
 * Locates a JSON string value by key.
 * @param json JSON document.
 * @param key Object member name.
 * @return Pointer to the value, or NULL when absent.
 */
static const char *webp2p_json_value(const char *json, const char *key)
{
    char needle[128];
    const char *at;
    size_t length;

    if (!json || !key) return NULL;
    length = strlen(key);
    if (length + 3 > sizeof(needle)) return NULL;
    (void)snprintf(needle, sizeof(needle), "\"%s\"", key);
    at = strstr(json, needle);
    if (!at) return NULL;
    at = webp2p_skip_space(at + strlen(needle));
    if (!at || *at != ':') return NULL;
    return webp2p_skip_space(at + 1);
}

/**
 * Converts one hexadecimal character to its numeric value.
 * @param value Hexadecimal character.
 * @return Value from 0 to 15, or -1 when invalid.
 */
static int webp2p_hex_value(char value)
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

/**
 * Converts a numeric value to a lowercase hexadecimal character.
 * @param value Numeric value.
 * @return Lowercase hexadecimal digit.
 */
static char webp2p_hex_digit(unsigned value)
{
    return "0123456789abcdef"[value & 15u];
}

/**
 * Encodes bytes as a hexadecimal string.
 * @param data Bytes to encode.
 * @param size Number of bytes.
 * @param out Destination string.
 * @return None.
 */
static void webp2p_hex_encode(const unsigned char *data, size_t size,
    char *out)
{
    size_t i;

    for (i = 0; i < size; i++) {
        out[i * 2] = webp2p_hex_digit(data[i] >> 4);
        out[i * 2 + 1] = webp2p_hex_digit(data[i]);
    }
    out[size * 2] = '\0';
}

/**
 * Decodes a hexadecimal string into bytes.
 * @param text Hexadecimal string.
 * @param out Destination bytes.
 * @param size Expected byte count.
 * @return 0 on success, or -1 for invalid input.
 */
static int webp2p_hex_decode(const char *text, unsigned char *out,
    size_t size)
{
    size_t i;

    if (!text || strlen(text) != size * 2) return -1;
    for (i = 0; i < size; i++) {
        int high = webp2p_hex_value(text[i * 2]);
        int low = webp2p_hex_value(text[i * 2 + 1]);
        if (high < 0 || low < 0) return -1;
        out[i] = (unsigned char)((high << 4) | low);
    }
    return 0;
}

/**
 * Checks whether text contains lowercase hexadecimal digits.
 * @param text Text to validate.
 * @param size Expected text length.
 * @return Nonzero when valid, otherwise 0.
 */
static int webp2p_hex_lower(const char *text, size_t size)
{
    size_t i;

    if (!text || strlen(text) != size) return 0;
    for (i = 0; i < size; i++) {
        if (!((text[i] >= '0' && text[i] <= '9') ||
            (text[i] >= 'a' && text[i] <= 'f')))
            return 0;
    }
    return 1;
}

/**
 * Parses a JSON string at the current position.
 * @param text JSON text.
 * @return Allocated decoded string, or NULL on failure.
 */
static char *webp2p_json_string_at(const char *text)
{
    char *out;
    size_t capacity = 128;
    size_t size = 0;

    text = webp2p_skip_space(text);
    if (!text || *text != '"') return NULL;
    text++;
    out = (char *)malloc(capacity);
    if (!out) return NULL;
    while (*text && *text != '"') {
        unsigned char value = (unsigned char)*text++;
        if (value == '\\') {
            value = (unsigned char)*text++;
            switch (value) {
                case '"': value = '"'; break;
                case '\\': value = '\\'; break;
                case '/': value = '/'; break;
                case 'b': value = '\b'; break;
                case 'f': value = '\f'; break;
                case 'n': value = '\n'; break;
                case 'r': value = '\r'; break;
                case 't': value = '\t'; break;
                case 'u': {
                    int a = webp2p_hex_value(text[0]);
                    int b = webp2p_hex_value(text[1]);
                    int c = webp2p_hex_value(text[2]);
                    int d = webp2p_hex_value(text[3]);
                    unsigned code;
                    if (a < 0 || b < 0 || c < 0 || d < 0) {
                        free(out);
                        return NULL;
                    }
                    code = (unsigned)((a << 12) | (b << 8) | (c << 4) | d);
                    text += 4;
                    value = code < 0x80 ? (unsigned char)code : '?';
                    break;
                }
                default:
                    free(out);
                    return NULL;
            }
        }
        if (size + 2 > capacity) {
            char *next;
            capacity *= 2;
            next = (char *)realloc(out, capacity);
            if (!next) {
                free(out);
                return NULL;
            }
            out = next;
        }
        out[size++] = (char)value;
    }
    if (*text != '"') {
        free(out);
        return NULL;
    }
    out[size] = '\0';
    return out;
}

/**
 * Extracts a JSON string member.
 * @return Allocated member string, or NULL when unavailable.
 */
static char *webp2p_json_string(const char *json, const char *key)
{
    return webp2p_json_string_at(webp2p_json_value(json, key));
}

/**
 * Extracts an unsigned JSON member.
 * @return 0 on success, or -1 on invalid input.
 */
static int webp2p_json_u64(const char *json, const char *key,
    uint64_t *value)
{
    const char *at = webp2p_json_value(json, key);
    unsigned long long parsed;
    char *end;

    if (!at || !isdigit((unsigned char)*at)) return -1;
    parsed = strtoull(at, &end, 10);
    if (end == at) return -1;
    *value = (uint64_t)parsed;
    return 0;
}

/**
 * Extracts a Boolean JSON member.
 * @return 0 on success, or -1 on invalid input.
 */
static int webp2p_json_bool(const char *json, const char *key, int *value)
{
    const char *at = webp2p_json_value(json, key);

    if (!at) return -1;
    if (strncmp(at, "true", 4) == 0) {
        *value = 1;
        return 0;
    }
    if (strncmp(at, "false", 5) == 0) {
        *value = 0;
        return 0;
    }
    return -1;
}

/**
 * Quotes text for a JSON string.
 * @return Allocated JSON string, or NULL on failure.
 */
static char *webp2p_json_quote(const char *text)
{
    size_t size = 2;
    size_t i;
    char *out;
    char *cursor;

    if (!text) return NULL;
    for (i = 0; text[i]; i++) {
        unsigned char value = (unsigned char)text[i];
        if (value == '"' || value == '\\' || value == '\b' ||
            value == '\f' || value == '\n' || value == '\r' ||
            value == '\t')
            size += 2;
        else if (value < 0x20)
            size += 6;
        else
            size++;
    }
    out = (char *)malloc(size + 1);
    if (!out) return NULL;
    cursor = out;
    *cursor++ = '"';
    for (i = 0; text[i]; i++) {
        unsigned char value = (unsigned char)text[i];
        switch (value) {
            case '"': *cursor++ = '\\'; *cursor++ = '"'; break;
            case '\\': *cursor++ = '\\'; *cursor++ = '\\'; break;
            case '\b': *cursor++ = '\\'; *cursor++ = 'b'; break;
            case '\f': *cursor++ = '\\'; *cursor++ = 'f'; break;
            case '\n': *cursor++ = '\\'; *cursor++ = 'n'; break;
            case '\r': *cursor++ = '\\'; *cursor++ = 'r'; break;
            case '\t': *cursor++ = '\\'; *cursor++ = 't'; break;
            default:
                if (value < 0x20) {
                    (void)snprintf(cursor, 7, "\\u%04x", value);
                    cursor += 6;
                } else {
                    *cursor++ = (char)value;
                }
                break;
        }
    }
    *cursor++ = '"';
    *cursor = '\0';
    return out;
}

/**
 * Appends an HTTP response chunk.
 * @return Number of bytes consumed.
 */
static size_t webp2p_http_write(char *data, size_t size, size_t count,
    void *userdata)
{
    webp2p_http_buffer_t *buffer = (webp2p_http_buffer_t *)userdata;
    size_t bytes = size * count;
    char *next;

    if (bytes > SIZE_MAX - buffer->size - 1) return 0;
    next = (char *)realloc(buffer->data, buffer->size + bytes + 1);
    if (!next) return 0;
    buffer->data = next;
    memcpy(buffer->data + buffer->size, data, bytes);
    buffer->size += bytes;
    buffer->data[buffer->size] = '\0';
    return bytes;
}

/**
 * Sends one JSON request to the index.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_request(const char *index, const char *body,
    char **response)
{
    CURL *curl;
    CURLcode code;
    struct curl_slist *headers = NULL;
    webp2p_http_buffer_t buffer;
    long status = 0;
    int ok = 0;
    char *error = NULL;

    if (!index || !body || !response) return -1;
    *response = NULL;
    memset(&buffer, 0, sizeof(buffer));
    curl = curl_easy_init();
    if (!curl) return -1;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!headers) {
        curl_easy_cleanup(curl);
        return -1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, index);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webp2p_http_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "webp2p/0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 30000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    code = curl_easy_perform(curl);
    (void)curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (code != CURLE_OK) {
        fprintf(stderr, "webp2p: index request failed: %s\n",
            curl_easy_strerror(code));
        free(buffer.data);
        return -1;
    }
    if (!buffer.data || status < 200 || status >= 300 ||
        webp2p_json_bool(buffer.data, "ok", &ok) != 0 || !ok)
    {
        if (buffer.data) error = webp2p_json_string(buffer.data, "error");
        fprintf(stderr, "webp2p: index error: %s (HTTP %ld)\n",
            error ? error : "invalid response", status);
        free(error);
        free(buffer.data);
        return -1;
    }
    *response = buffer.data;
    return 0;
}

/**
 * Stores a 16-bit value in big-endian order.
 * @return None.
 */
static void webp2p_be16(unsigned char out[2], uint16_t value)
{
    out[0] = (unsigned char)(value >> 8);
    out[1] = (unsigned char)value;
}

/**
 * Stores a 64-bit value in big-endian order.
 * @return None.
 */
static void webp2p_be64(unsigned char out[8], uint64_t value)
{
    int i;

    for (i = 7; i >= 0; i--) {
        out[i] = (unsigned char)value;
        value >>= 8;
    }
}

/**
 * Computes a SHA-256 digest.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_sha256(const void *data, size_t size,
    unsigned char digest[32])
{
    unsigned length = 0;

    return EVP_Digest(data, size, digest, &length, EVP_sha256(), NULL) == 1 &&
        length == 32 ? 0 : -1;
}

/**
 * Computes a hexadecimal SHA-256 digest.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_sha256_hex(const void *data, size_t size, char out[65])
{
    unsigned char digest[32];

    if (webp2p_sha256(data, size, digest) != 0) return -1;
    webp2p_hex_encode(digest, sizeof(digest), out);
    return 0;
}

/**
 * Computes a hexadecimal HMAC digest.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_hmac_hex(const char *key, const void *data, size_t size,
    char out[65])
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned length = 0;

    if (!HMAC(EVP_sha256(), key, (int)strlen(key),
        (const unsigned char *)data, size, digest, &length) || length != 32)
        return -1;
    webp2p_hex_encode(digest, length, out);
    return 0;
}

/**
 * Counts leading zero bits in a digest.
 * @return Number of leading zero bits.
 */
static unsigned webp2p_leading_zero_bits(const unsigned char digest[32])
{
    unsigned bits = 0;
    size_t i;

    for (i = 0; i < 32; i++) {
        unsigned char value = digest[i];
        if (value == 0) {
            bits += 8;
            continue;
        }
        while ((value & 0x80u) == 0) {
            bits++;
            value <<= 1;
        }
        break;
    }
    return bits;
}

/**
 * Produces random bytes encoded as hexadecimal.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_random_hex(size_t bytes, char *out)
{
    unsigned char random[64];

    if (bytes > sizeof(random) || RAND_bytes(random, (int)bytes) != 1)
        return -1;
    webp2p_hex_encode(random, bytes, out);
    return 0;
}

/**
 * Parses a proof-of-work challenge.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_challenge_parse(const char *json,
    webp2p_challenge_t *challenge)
{
    char *nonce = webp2p_json_string(json, "nonce");
    char *mac = webp2p_json_string(json, "mac");
    uint64_t bits;
    int result = -1;

    if (!nonce || !mac || !webp2p_hex_lower(nonce, 64) ||
        !webp2p_hex_lower(mac, 64))
        goto done;
    if (webp2p_json_u64(json, "issued_at", &challenge->issued_at) != 0 ||
        webp2p_json_u64(json, "expires_at", &challenge->expires_at) != 0 ||
        webp2p_json_u64(json, "bits", &bits) != 0 || bits > 32)
        goto done;
    if (challenge->expires_at < challenge->issued_at ||
        challenge->expires_at - challenge->issued_at != 60)
        goto done;
    memcpy(challenge->nonce, nonce, 65);
    memcpy(challenge->mac, mac, 65);
    challenge->bits = (unsigned)bits;
    result = 0;
done:
    free(nonce);
    free(mac);
    return result;
}

/**
 * Solves a proof-of-work challenge.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_solve_pow(const char *id,
    const webp2p_challenge_t *challenge, char solution[17])
{
    unsigned char nonce[32];
    unsigned char input[256];
    unsigned char digest[32];
    unsigned char temporary[8];
    unsigned char *cursor;
    size_t id_size = strlen(id);
    uint64_t value;

    if (id_size > 65535 ||
        webp2p_hex_decode(challenge->nonce, nonce, sizeof(nonce)) != 0)
        return -1;
    for (value = 0;; value++) {
        cursor = input;
        memcpy(cursor, "REDP2P-POW", 10); cursor += 10;
        memcpy(cursor, nonce, 32); cursor += 32;
        webp2p_be64(temporary, challenge->issued_at);
        memcpy(cursor, temporary, 8); cursor += 8;
        webp2p_be64(temporary, challenge->expires_at);
        memcpy(cursor, temporary, 8); cursor += 8;
        webp2p_be16(temporary, (uint16_t)id_size);
        memcpy(cursor, temporary, 2); cursor += 2;
        memcpy(cursor, id, id_size); cursor += id_size;
        webp2p_be64(temporary, value);
        memcpy(cursor, temporary, 8); cursor += 8;
        if (webp2p_sha256(input, (size_t)(cursor - input), digest) != 0)
            return -1;
        if (webp2p_leading_zero_bits(digest) >= challenge->bits) {
            (void)snprintf(solution, 17, "%016" PRIx64, value);
            return 0;
        }
        if (value == UINT64_MAX) break;
    }
    return -1;
}

/**
 * Builds the canonical publisher registration payload.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_register_canonical(const char *id, const char *secret,
    const webp2p_challenge_t *challenge, const char *solution,
    unsigned char **data, size_t *size)
{
    unsigned char nonce[32];
    unsigned char solution_bytes[8];
    unsigned char temporary[8];
    const size_t prefix_size = sizeof(WEBP2P_REGISTER_PREFIX) - 1;
    size_t id_size = strlen(id);
    size_t secret_size = strlen(secret);
    unsigned char *out;
    unsigned char *cursor;

    if (id_size > 65535 || secret_size > 65535 ||
        webp2p_hex_decode(challenge->nonce, nonce, sizeof(nonce)) != 0 ||
        webp2p_hex_decode(solution, solution_bytes,
            sizeof(solution_bytes)) != 0)
        return -1;
    *size = prefix_size + 32 + 8 + 8 + 2 + id_size + 2 + secret_size + 8;
    out = (unsigned char *)malloc(*size);
    if (!out) return -1;
    cursor = out;
    memcpy(cursor, WEBP2P_REGISTER_PREFIX, prefix_size); cursor += prefix_size;
    memcpy(cursor, nonce, 32); cursor += 32;
    webp2p_be64(temporary, challenge->issued_at);
    memcpy(cursor, temporary, 8); cursor += 8;
    webp2p_be64(temporary, challenge->expires_at);
    memcpy(cursor, temporary, 8); cursor += 8;
    webp2p_be16(temporary, (uint16_t)id_size);
    memcpy(cursor, temporary, 2); cursor += 2;
    memcpy(cursor, id, id_size); cursor += id_size;
    webp2p_be16(temporary, (uint16_t)secret_size);
    memcpy(cursor, temporary, 2); cursor += 2;
    memcpy(cursor, secret, secret_size); cursor += secret_size;
    memcpy(cursor, solution_bytes, 8);
    *data = out;
    return 0;
}

/**
 * Computes a publisher control proof.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_control_proof(const webp2p_publisher_t *publisher,
    const char *operation, const char *id, uint64_t seq, const char *extra1,
    const char *extra2, char out[65])
{
    char *canonical;
    int result;

    if (extra2)
        canonical = webp2p_format("%s\n%s\n%" PRIu64 "\n%s\n%s",
            operation, id, seq, extra1, extra2);
    else if (extra1)
        canonical = webp2p_format("%s\n%s\n%" PRIu64 "\n%s",
            operation, id, seq, extra1);
    else
        canonical = webp2p_format("%s\n%s\n%" PRIu64,
            operation, id, seq);
    if (!canonical) return -1;
    result = webp2p_hmac_hex(publisher->secret, canonical,
        strlen(canonical), out);
    free(canonical);
    return result;
}

/**
 * Extracts the next JSON object from a cursor.
 * @return Allocated JSON object, or NULL when unavailable.
 */
static char *webp2p_json_object(const char **cursor)
{
    const char *start;
    const char *at;
    int depth = 0;
    int quoted = 0;
    int escaped = 0;
    char *out;
    size_t size;

    at = webp2p_skip_space(*cursor);
    if (!at || *at != '{') return NULL;
    start = at;
    for (; *at; at++) {
        char value = *at;
        if (quoted) {
            if (escaped) escaped = 0;
            else if (value == '\\') escaped = 1;
            else if (value == '"') quoted = 0;
            continue;
        }
        if (value == '"') quoted = 1;
        else if (value == '{') depth++;
        else if (value == '}' && --depth == 0) {
            size = (size_t)(at - start) + 1;
            out = (char *)malloc(size + 1);
            if (!out) return NULL;
            memcpy(out, start, size);
            out[size] = '\0';
            *cursor = at + 1;
            return out;
        }
    }
    return NULL;
}

/**
 * Initializes protocol dependencies.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_protocol_init(void)
{
    return curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK ? 0 : -1;
}

/**
 * Cleans up protocol dependencies.
 * @return None.
 */
void webp2p_protocol_cleanup(void)
{
    curl_global_cleanup();
}

/**
 * Gets the current Unix time in milliseconds.
 * @return Current time in milliseconds.
 */
uint64_t webp2p_now_ms(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_REALTIME, &now) != 0) return 0;
    return (uint64_t)now.tv_sec * 1000u + (uint64_t)now.tv_nsec / 1000000u;
}

/**
 * Validates a WebP2P identifier.
 * @return Nonzero when valid, otherwise 0.
 */
int webp2p_validate_id(const char *id)
{
    size_t i;
    size_t size;

    if (!id) return -1;
    size = strlen(id);
    if (size == 0 || size > 63) return -1;
    for (i = 0; i < size; i++)
        if (!isalnum((unsigned char)id[i])) return -1;
    return 0;
}

/**
 * Frees an RTC description.
 * @return None.
 */
void webp2p_description_free(webp2p_description_t *description)
{
    if (!description) return;
    free(description->type);
    free(description->sdp);
    description->type = NULL;
    description->sdp = NULL;
}

/**
 * Frees a pending connection.
 * @return None.
 */
void webp2p_pending_free(webp2p_pending_t *pending)
{
    if (!pending) return;
    webp2p_description_free(&pending->offer);
    memset(pending->connection, 0, sizeof(pending->connection));
}

/**
 * Registers a publisher with the index.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_pub_register(webp2p_publisher_t *publisher, const char *index,
    const char *id, const char *pass)
{
    webp2p_challenge_t challenge;
    char solution[17];
    unsigned char *canonical = NULL;
    size_t canonical_size = 0;
    char proof[65];
    char access_proof[65];
    char *qid = NULL;
    char *body = NULL;
    char *response = NULL;
    int result = -1;

    if (!publisher || !index || webp2p_validate_id(id) != 0) return -1;
    memset(publisher, 0, sizeof(*publisher));
    publisher->index = index;
    if (webp2p_random_hex(8, publisher->secret) != 0) return -1;
    qid = webp2p_json_quote(id);
    if (!qid) goto done;
    body = webp2p_format(
        "{\"op\":\"challenge\",\"id\":%s,\"version\":0}", qid);
    if (!body || webp2p_request(index, body, &response) != 0 ||
        webp2p_challenge_parse(response, &challenge) != 0)
        goto done;
    free(body); body = NULL;
    free(response); response = NULL;
    if (webp2p_solve_pow(id, &challenge, solution) != 0 ||
        webp2p_register_canonical(id, publisher->secret, &challenge,
            solution, &canonical, &canonical_size) != 0 ||
        webp2p_hmac_hex(publisher->secret, canonical, canonical_size,
            proof) != 0)
        goto done;
    if (pass && pass[0]) {
        const size_t prefix_size = sizeof(WEBP2P_ADMISSION_PREFIX) - 1;
        unsigned char *admission = (unsigned char *)malloc(
            prefix_size + canonical_size);
        if (!admission) goto done;
        memcpy(admission, WEBP2P_ADMISSION_PREFIX, prefix_size);
        memcpy(admission + prefix_size, canonical, canonical_size);
        if (webp2p_hmac_hex(pass, admission, prefix_size + canonical_size,
            access_proof) != 0)
        {
            free(admission);
            goto done;
        }
        free(admission);
        body = webp2p_format(
            "{\"op\":\"register\",\"id\":%s,\"nonce\":\"%s\","
            "\"issued_at\":%" PRIu64 ",\"expires_at\":%" PRIu64 ","
            "\"secret\":\"%s\",\"pow_solution\":\"%s\","
            "\"transport\":\"rtc\",\"mac\":\"%s\","
            "\"proof\":\"%s\",\"access_proof\":\"%s\",\"version\":0}",
            qid, challenge.nonce, challenge.issued_at, challenge.expires_at,
            publisher->secret, solution, challenge.mac, proof, access_proof);
    } else {
        body = webp2p_format(
            "{\"op\":\"register\",\"id\":%s,\"nonce\":\"%s\","
            "\"issued_at\":%" PRIu64 ",\"expires_at\":%" PRIu64 ","
            "\"secret\":\"%s\",\"pow_solution\":\"%s\","
            "\"transport\":\"rtc\",\"mac\":\"%s\","
            "\"proof\":\"%s\",\"version\":0}", qid,
            challenge.nonce, challenge.issued_at, challenge.expires_at,
            publisher->secret, solution, challenge.mac, proof);
    }
    if (!body || webp2p_request(index, body, &response) != 0) goto done;
    result = 0;
done:
    free(qid);
    free(body);
    free(response);
    free(canonical);
    return result;
}

/**
 * Sends a publisher control request.
 * @return 0 on success, or -1 on failure.
 */
static int webp2p_pub_control(webp2p_publisher_t *publisher,
    const char *id, const char *operation)
{
    char proof[65];
    char *qid;
    char *body;
    char *response = NULL;
    int result;

    publisher->seq++;
    if (webp2p_control_proof(publisher, operation, id, publisher->seq,
        NULL, NULL, proof) != 0)
        return -1;
    qid = webp2p_json_quote(id);
    if (!qid) return -1;
    body = webp2p_format(
        "{\"op\":\"%s\",\"id\":%s,\"seq\":%" PRIu64 ","
        "\"proof\":\"%s\",\"version\":0}", operation, qid,
        publisher->seq, proof);
    free(qid);
    if (!body) return -1;
    result = webp2p_request(publisher->index, body, &response);
    free(body);
    free(response);
    return result;
}

/**
 * Sends a publisher heartbeat.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_pub_heartbeat(webp2p_publisher_t *publisher, const char *id)
{
    return webp2p_pub_control(publisher, id, "heartbeat");
}

/**
 * Deregisters a publisher.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_pub_deregister(webp2p_publisher_t *publisher, const char *id)
{
    return webp2p_pub_control(publisher, id, "deregister");
}

/**
 * Polls for publisher work.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_pub_poll(webp2p_publisher_t *publisher, const char *id,
    webp2p_pending_t pending[WEBP2P_MAX_PENDING], size_t *count)
{
    char proof[65];
    char *qid = NULL;
    char *body = NULL;
    char *response = NULL;
    const char *cursor;
    size_t total = 0;
    int result = -1;

    if (!publisher || !pending || !count) return -1;
    memset(pending, 0, sizeof(*pending) * WEBP2P_MAX_PENDING);
    *count = 0;
    publisher->seq++;
    if (webp2p_control_proof(publisher, "poll", id, publisher->seq,
        NULL, NULL, proof) != 0)
        return -1;
    qid = webp2p_json_quote(id);
    if (!qid) goto done;
    body = webp2p_format(
        "{\"op\":\"poll\",\"id\":%s,\"seq\":%" PRIu64 ","
        "\"proof\":\"%s\",\"version\":0}", qid, publisher->seq,
        proof);
    if (!body || webp2p_request(publisher->index, body, &response) != 0)
        goto done;
    cursor = webp2p_json_value(response, "connections");
    if (!cursor || *cursor != '[') goto done;
    cursor++;
    while (1) {
        char *object;
        char *connection;
        char *type;
        char *sdp;

        cursor = webp2p_skip_space(cursor);
        if (*cursor == ']') break;
        if (total >= WEBP2P_MAX_PENDING) goto done;
        object = webp2p_json_object(&cursor);
        if (!object) goto done;
        connection = webp2p_json_string(object, "connection");
        type = webp2p_json_string(object, "type");
        sdp = webp2p_json_string(object, "sdp");
        free(object);
        if (!connection || !webp2p_hex_lower(connection, 32) || !type ||
            strcmp(type, "offer") != 0 || !sdp || !sdp[0] ||
            strlen(sdp) > 49152)
        {
            free(connection); free(type); free(sdp);
            goto done;
        }
        memcpy(pending[total].connection, connection, 33);
        pending[total].offer.type = type;
        pending[total].offer.sdp = sdp;
        free(connection);
        total++;
        cursor = webp2p_skip_space(cursor);
        if (*cursor == ',') {
            cursor++;
            continue;
        }
        if (*cursor != ']') goto done;
    }
    *count = total;
    result = 0;
done:
    if (result != 0) {
        size_t i;
        for (i = 0; i < WEBP2P_MAX_PENDING; i++)
            webp2p_pending_free(&pending[i]);
    }
    free(qid);
    free(body);
    free(response);
    return result;
}

/**
 * Submits a publisher answer.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_pub_answer(webp2p_publisher_t *publisher, const char *id,
    const char *connection, const webp2p_description_t *answer)
{
    char digest[65];
    char proof[65];
    char *digest_input = NULL;
    char *qid = NULL;
    char *qconnection = NULL;
    char *qtype = NULL;
    char *qsdp = NULL;
    char *body = NULL;
    char *response = NULL;
    int result = -1;

    if (!publisher || !answer || !answer->type || !answer->sdp ||
        !webp2p_hex_lower(connection, 32))
        return -1;
    digest_input = webp2p_format("%s\n%s", answer->type, answer->sdp);
    if (!digest_input || webp2p_sha256_hex(digest_input,
        strlen(digest_input), digest) != 0)
        goto done;
    publisher->seq++;
    if (webp2p_control_proof(publisher, "answer", id, publisher->seq,
        connection, digest, proof) != 0)
        goto done;
    qid = webp2p_json_quote(id);
    qconnection = webp2p_json_quote(connection);
    qtype = webp2p_json_quote(answer->type);
    qsdp = webp2p_json_quote(answer->sdp);
    if (!qid || !qconnection || !qtype || !qsdp) goto done;
    body = webp2p_format(
        "{\"op\":\"answer\",\"id\":%s,\"connection\":%s,"
        "\"answer\":{\"type\":%s,\"sdp\":%s},\"seq\":%" PRIu64 ","
        "\"proof\":\"%s\",\"version\":0}", qid, qconnection,
        qtype, qsdp, publisher->seq, proof);
    if (!body || webp2p_request(publisher->index, body, &response) != 0)
        goto done;
    result = 0;
done:
    free(digest_input); free(qid); free(qconnection); free(qtype); free(qsdp);
    free(body); free(response);
    return result;
}

/**
 * Requests a consumer connection.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_con_connect(const char *index, const char *id,
    const webp2p_description_t *offer, webp2p_connection_t *connection)
{
    char *qid = NULL;
    char *qtype = NULL;
    char *qsdp = NULL;
    char *body = NULL;
    char *response = NULL;
    char *connection_text = NULL;
    char *capability = NULL;
    int result = -1;

    if (!index || webp2p_validate_id(id) != 0 || !offer || !offer->type ||
        !offer->sdp || !connection)
        return -1;
    memset(connection, 0, sizeof(*connection));
    qid = webp2p_json_quote(id);
    qtype = webp2p_json_quote(offer->type);
    qsdp = webp2p_json_quote(offer->sdp);
    if (!qid || !qtype || !qsdp) goto done;
    body = webp2p_format(
        "{\"op\":\"connect\",\"id\":%s,"
        "\"offer\":{\"type\":%s,\"sdp\":%s},\"version\":0}",
        qid, qtype, qsdp);
    if (!body || webp2p_request(index, body, &response) != 0) goto done;
    connection_text = webp2p_json_string(response, "connection");
    capability = webp2p_json_string(response, "capability");
    if (!webp2p_hex_lower(connection_text, 32) ||
        !webp2p_hex_lower(capability, 64) ||
        webp2p_json_u64(response, "expires_at", &connection->expires_at) != 0)
        goto done;
    memcpy(connection->connection, connection_text, 33);
    memcpy(connection->capability, capability, 65);
    result = 0;
done:
    free(qid); free(qtype); free(qsdp); free(body); free(response);
    free(connection_text); free(capability);
    return result;
}

/**
 * Polls a consumer connection.
 * @return 0 on success, or -1 on failure.
 */
int webp2p_con_poll(const char *index, const webp2p_connection_t *connection,
    webp2p_description_t *answer, int *has_answer)
{
    char *qconnection = NULL;
    char *qcapability = NULL;
    char *body = NULL;
    char *response = NULL;
    const char *value;
    char *type = NULL;
    char *sdp = NULL;
    int result = -1;

    if (!index || !connection || !answer || !has_answer) return -1;
    memset(answer, 0, sizeof(*answer));
    *has_answer = 0;
    qconnection = webp2p_json_quote(connection->connection);
    qcapability = webp2p_json_quote(connection->capability);
    if (!qconnection || !qcapability) goto done;
    body = webp2p_format(
        "{\"op\":\"poll\",\"connection\":%s,\"capability\":%s,"
        "\"version\":0}", qconnection, qcapability);
    if (!body || webp2p_request(index, body, &response) != 0) goto done;
    value = webp2p_json_value(response, "answer");
    if (!value) goto done;
    if (strncmp(value, "null", 4) == 0) {
        result = 0;
        goto done;
    }
    type = webp2p_json_string(value, "type");
    sdp = webp2p_json_string(value, "sdp");
    if (!type || strcmp(type, "answer") != 0 || !sdp || !sdp[0] ||
        strlen(sdp) > 49152)
        goto done;
    answer->type = type; type = NULL;
    answer->sdp = sdp; sdp = NULL;
    *has_answer = 1;
    result = 0;
done:
    free(qconnection); free(qcapability); free(body); free(response);
    free(type); free(sdp);
    return result;
}
