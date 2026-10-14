/**
 * hpm.c - Hole-Punch-Man command-line interface.
 * Summary: Sends or receives one datagram over a direct libhpm UDP path.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "libhpm.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

/**
 * Prints the CLI usage message.
 * @param name Program name.
 * @return None.
 */
static void hpm_cli_usage(const char *name)
{
    printf("Usage: %s <send|receive> <peer> [peer...] [options]\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  --host <host>           Local bind host\n");
    printf("  -p, --port <port>       Local UDP port (default: 0)\n");
    printf("  -t, --timeout <ms>      Connection timeout in ms (default: library)\n");
    printf("  -i, --input <file>      Read send datagram from file\n");
    printf("  --receive-timeout <ms>  Receive timeout in ms (default: 0, infinite)\n");
    printf("  -h, --help              Show this help\n");
    printf("  -v, --version           Show version\n");
}

/**
 * Checks if stdin is a TTY.
 * @return 1 if stdin is a TTY, 0 otherwise.
 */
static int hpm_cli_stdin_is_tty(void)
{
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;

    if (handle == INVALID_HANDLE_VALUE || handle == NULL) return 0;
    return GetConsoleMode(handle, &mode) != 0;
#else
    return isatty(STDIN_FILENO);
#endif
}

/**
 * Reads all bytes from a stream.
 * @param stream Input stream.
 * @param out Output buffer.
 * @param out_size Output byte count.
 * @return Zero on success.
 */
static int hpm_cli_read_stream(FILE *stream, void **out, size_t *out_size)
{
    unsigned char *buffer = NULL;
    size_t size = 0;
    size_t capacity = 0;
    unsigned char chunk[4096];
    size_t count;

    if (!stream || !out || !out_size) return 1;
    *out = NULL;
    *out_size = 0;

    while ((count = fread(chunk, 1, sizeof(chunk), stream)) > 0) {
        size_t required;
        unsigned char *next;

        if (count > SIZE_MAX - size) {
            free(buffer);
            return 1;
        }
        required = size + count;
        if (required > capacity) {
            size_t next_capacity = capacity ? capacity : 4096U;
            while (next_capacity < required) {
                if (next_capacity > SIZE_MAX / 2U) {
                    next_capacity = required;
                    break;
                }
                next_capacity *= 2U;
            }
            next = (unsigned char *)realloc(buffer, next_capacity);
            if (!next) {
                free(buffer);
                return 1;
            }
            buffer = next;
            capacity = next_capacity;
        }
        memcpy(buffer + size, chunk, count);
        size += count;
    }

    if (ferror(stream)) {
        free(buffer);
        return 1;
    }
    if (!buffer) {
        buffer = (unsigned char *)malloc(1U);
        if (!buffer) return 1;
    }
    *out = buffer;
    *out_size = size;
    return 0;
}

/**
 * Reads a file into memory.
 * @param path File path.
 * @param out Output buffer.
 * @param out_size Output byte count.
 * @return Zero on success.
 */
static int hpm_cli_read_file(const char *path, void **out, size_t *out_size)
{
    FILE *file;
    int status;

    if (!path) return 1;
    file = fopen(path, "rb");
    if (!file) return 1;
    status = hpm_cli_read_stream(file, out, out_size);
    fclose(file);
    return status;
}

/**
 * Reads one send datagram from stdin or a file.
 * @param path Optional input file.
 * @param out Output buffer.
 * @param out_size Output byte count.
 * @return Zero on success.
 */
static int hpm_cli_read_input(const char *path, void **out, size_t *out_size)
{
    if (!hpm_cli_stdin_is_tty()) {
#ifdef _WIN32
        if (_setmode(_fileno(stdin), _O_BINARY) == -1) return 1;
#endif
        if (hpm_cli_read_stream(stdin, out, out_size)) return 1;
        if (*out_size > 0U || !path) return 0;
        free(*out);
        *out = NULL;
    }
    if (path) return hpm_cli_read_file(path, out, out_size);
    return 1;
}

/**
 * Writes one binary datagram to stdout.
 * @param data Datagram bytes.
 * @param size Datagram size.
 * @return Zero on success.
 */
static int hpm_cli_write_stdout(const void *data, size_t size)
{
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) return 1;
#endif
    if (size > 0U && fwrite(data, 1, size, stdout) != size) return 1;
    return fflush(stdout) == 0 ? 0 : 1;
}

/**
 * Parses an unsigned 64-bit decimal value.
 * @param text Input text.
 * @param out Parsed value.
 * @return Nonzero on success.
 */
static int hpm_cli_u64(const char *text, uint64_t *out)
{
    unsigned long long value;
    char *end = NULL;

    if (!text || !text[0] || !out || text[0] == '-') return 0;
    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno || !end || *end != '\0') return 0;
    *out = (uint64_t)value;
    return 1;
}

/**
 * Parses a UDP port.
 * @param text Input text.
 * @param allow_zero Whether zero is accepted.
 * @param out Parsed port.
 * @return Nonzero on success.
 */
static int hpm_cli_port(const char *text, int allow_zero, unsigned short *out)
{
    uint64_t value;

    if (!hpm_cli_u64(text, &value) || value > 65535ULL) return 0;
    if (!allow_zero && value == 0ULL) return 0;
    *out = (unsigned short)value;
    return 1;
}

/**
 * Parses one host:port or [ipv6]:port peer endpoint.
 * @param text Endpoint text.
 * @param out Output endpoint.
 * @return Zero on success.
 */
static int hpm_cli_endpoint(const char *text, kc_hpm_endpoint_t *out)
{
    const char *host_begin;
    const char *host_end;
    const char *port_text;
    size_t host_size;
    char *host;
    unsigned short port;

    if (!text || !text[0] || !out) return 1;
    if (text[0] == '[') {
        host_begin = text + 1;
        host_end = strchr(host_begin, ']');
        if (!host_end || host_end == host_begin || host_end[1] != ':' ||
            !host_end[2]) return 1;
        port_text = host_end + 2;
    } else {
        const char *colon = strrchr(text, ':');
        if (!colon || colon == text || !colon[1]) return 1;
        if (memchr(text, ':', (size_t)(colon - text)) != NULL) return 1;
        host_begin = text;
        host_end = colon;
        port_text = colon + 1;
    }
    if (!hpm_cli_port(port_text, 0, &port)) return 1;
    host_size = (size_t)(host_end - host_begin);
    host = (char *)malloc(host_size + 1U);
    if (!host) return 1;
    memcpy(host, host_begin, host_size);
    host[host_size] = '\0';
    out->host = host;
    out->port = port;
    return 0;
}

/**
 * Releases parsed peer endpoint hosts.
 * @param peers Peer array.
 * @param count Peer count.
 * @return None.
 */
static void hpm_cli_free_peers(kc_hpm_endpoint_t *peers, size_t count)
{
    size_t i;
    if (!peers) return;
    for (i = 0; i < count; ++i) free((void *)peers[i].host);
    free(peers);
}

/**
 * Establishes a path and sends one datagram.
 * @param options HPM connection options.
 * @param input_path Optional input file.
 * @return Process exit status.
 */
static int hpm_cli_send(const kc_hpm_options_t *options, const char *input_path)
{
    kc_hpm_path_t *path = NULL;
    void *data = NULL;
    size_t size = 0;
    int rc;

    if (hpm_cli_read_input(input_path, &data, &size)) {
        fprintf(stderr, "hpm: cannot read datagram\n");
        return 1;
    }
    rc = kc_hpm_connect(&path, options);
    if (rc != KC_HPM_OK) {
        fprintf(stderr, "hpm: connect failed: %s\n", kc_hpm_strerror(rc));
        free(data);
        return 1;
    }
    rc = kc_hpm_send(path, data, size);
    free(data);
    kc_hpm_close(path);
    if (rc != KC_HPM_OK) {
        fprintf(stderr, "hpm: send failed: %s\n", kc_hpm_strerror(rc));
        return 1;
    }
    return 0;
}

/**
 * Establishes a path and receives one datagram.
 * @param options HPM connection options.
 * @param receive_timeout_ms Receive timeout in milliseconds.
 * @return Process exit status.
 */
static int hpm_cli_receive(const kc_hpm_options_t *options,
    uint64_t receive_timeout_ms)
{
    kc_hpm_path_t *path = NULL;
    void *data = NULL;
    size_t size = 0;
    int rc;

    rc = kc_hpm_connect(&path, options);
    if (rc != KC_HPM_OK) {
        fprintf(stderr, "hpm: connect failed: %s\n", kc_hpm_strerror(rc));
        return 1;
    }
    rc = kc_hpm_receive(path, &data, &size, receive_timeout_ms);
    kc_hpm_close(path);
    if (rc != KC_HPM_OK) {
        fprintf(stderr, "hpm: receive failed: %s\n", kc_hpm_strerror(rc));
        return 1;
    }
    if (hpm_cli_write_stdout(data, size)) {
        fprintf(stderr, "hpm: cannot write datagram\n");
        kc_hpm_free(data);
        return 1;
    }
    kc_hpm_free(data);
    return 0;
}

/**
 * Main entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit status.
 */
int main(int argc, char **argv)
{
    const char *command = NULL;
    const char *host = NULL;
    const char *input_path = NULL;
    const char **peer_args = NULL;
    size_t peer_count = 0;
    kc_hpm_endpoint_t *peers = NULL;
    kc_hpm_options_t options;
    unsigned short port = 0;
    uint64_t timeout_ms = 0;
    uint64_t receive_timeout_ms = 0;
    int rc = 1;
    int i;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0)) {
        hpm_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0)) {
        printf("%llu\n", (unsigned long long)kc_hpm_version());
        return 0;
    }
    if (argc < 2) {
        hpm_cli_usage(argv[0]);
        return 1;
    }

    peer_args = (const char **)calloc((size_t)argc, sizeof(*peer_args));
    if (!peer_args) {
        fprintf(stderr, "hpm: out of memory\n");
        return 1;
    }

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--host") == 0) {
            if (++i >= argc || !argv[i][0] || host) {
                fprintf(stderr, "hpm: --host requires one host\n");
                goto done;
            }
            host = argv[i];
        } else if (strcmp(argv[i], "-p") == 0 ||
            strcmp(argv[i], "--port") == 0) {
            if (++i >= argc || !hpm_cli_port(argv[i], 1, &port)) {
                fprintf(stderr, "hpm: invalid port\n");
                goto done;
            }
        } else if (strcmp(argv[i], "-t") == 0 ||
            strcmp(argv[i], "--timeout") == 0) {
            if (++i >= argc || !hpm_cli_u64(argv[i], &timeout_ms)) {
                fprintf(stderr, "hpm: invalid timeout\n");
                goto done;
            }
        } else if (strcmp(argv[i], "--receive-timeout") == 0) {
            if (++i >= argc || !hpm_cli_u64(argv[i], &receive_timeout_ms)) {
                fprintf(stderr, "hpm: invalid receive timeout\n");
                goto done;
            }
        } else if (strcmp(argv[i], "-i") == 0 ||
            strcmp(argv[i], "--input") == 0) {
            if (++i >= argc || !argv[i][0] || input_path) {
                fprintf(stderr, "hpm: --input requires one file\n");
                goto done;
            }
            input_path = argv[i];
        } else if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0) {
            fprintf(stderr, "hpm: -h/--help and -v/--version must be used alone\n");
            goto done;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "hpm: unknown option: %s\n", argv[i]);
            goto done;
        } else if (!command) {
            command = argv[i];
        } else {
            peer_args[peer_count++] = argv[i];
        }
    }

    if (!command) {
        fprintf(stderr, "hpm: missing command\n");
        goto done;
    }
    if (strcmp(command, "send") != 0 && strcmp(command, "receive") != 0) {
        fprintf(stderr, "hpm: unknown command '%s'\n", command);
        goto done;
    }
    if (peer_count == 0U) {
        fprintf(stderr, "hpm: %s requires at least one peer\n", command);
        goto done;
    }
    if (strcmp(command, "send") == 0 && receive_timeout_ms != 0U) {
        fprintf(stderr, "hpm: send does not use --receive-timeout\n");
        goto done;
    }
    if (strcmp(command, "receive") == 0 && input_path) {
        fprintf(stderr, "hpm: receive does not use --input\n");
        goto done;
    }

    peers = (kc_hpm_endpoint_t *)calloc(peer_count, sizeof(*peers));
    if (!peers) {
        fprintf(stderr, "hpm: out of memory\n");
        goto done;
    }
    for (i = 0; (size_t)i < peer_count; ++i) {
        if (hpm_cli_endpoint(peer_args[i], &peers[i])) {
            fprintf(stderr, "hpm: invalid peer endpoint: %s\n", peer_args[i]);
            goto done;
        }
    }

    memset(&options, 0, sizeof(options));
    options.host = host;
    options.port = port;
    options.peers = peers;
    options.peer_count = peer_count;
    options.timeout_ms = timeout_ms;

    if (strcmp(command, "send") == 0)
        rc = hpm_cli_send(&options, input_path);
    else
        rc = hpm_cli_receive(&options, receive_timeout_ms);

done:
    hpm_cli_free_peers(peers, peer_count);
    free(peer_args);
    return rc;
}
