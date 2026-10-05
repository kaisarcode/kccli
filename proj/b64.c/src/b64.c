/**
 * b64.c - Base64 command-line interface.
 * Summary: Encodes or decodes Base64 with libb64.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "libb64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define B64_MODE_NONE   0
#define B64_MODE_ENCODE 1
#define B64_MODE_DECODE 2

/**
 * Prints public CLI usage.
 * @param name Program executable name.
 * @return None.
 */
static void b64_cli_usage(const char *name)
{
    printf("Usage: %s -e [text]\n", name);
    printf("       %s -d [base64]\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  -e, --encode   Encode as Base64\n");
    printf("  -d, --decode   Decode Base64\n");
    printf("  -h, --help     Show this help\n");
    printf("  -v, --version  Show version\n");
}

/**
 * Reads all bytes from standard input.
 * @param out Receives allocated data.
 * @param out_size Receives byte count.
 * @return Zero on success, nonzero on failure.
 */
static int b64_cli_read_stdin(unsigned char **out, size_t *out_size)
{
    unsigned char *buffer = NULL;
    size_t size = 0;
    size_t capacity = 0;
    unsigned char chunk[4096];
    size_t count;

    *out = NULL;
    *out_size = 0;
    while ((count = fread(chunk, 1, sizeof(chunk), stdin)) > 0) {
        size_t required = size + count;
        unsigned char *next;

        if (required > capacity) {
            size_t next_capacity = capacity ? capacity * 2 : 4096;

            while (next_capacity < required) next_capacity *= 2;
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
    if (ferror(stdin)) {
        free(buffer);
        return 1;
    }
    if (!buffer) {
        buffer = (unsigned char *)malloc(1);
        if (!buffer) return 1;
    }
    *out = buffer;
    *out_size = size;
    return 0;
}

/**
 * Runs the b64 CLI.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    const char *text = NULL;
    unsigned char *stdin_data = NULL;
    size_t stdin_size = 0;
    int mode = B64_MODE_NONE;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0))
    {
        b64_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0))
    {
        printf("%llu\n", (unsigned long long)kc_b64_version());
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--encode") == 0) {
            if (mode != B64_MODE_NONE) {
                fprintf(stderr, "b64: choose exactly one of --encode or --decode\n");
                return 1;
            }
            mode = B64_MODE_ENCODE;
        } else if (strcmp(argv[i], "-d") == 0 ||
            strcmp(argv[i], "--decode") == 0)
        {
            if (mode != B64_MODE_NONE) {
                fprintf(stderr, "b64: choose exactly one of --encode or --decode\n");
                return 1;
            }
            mode = B64_MODE_DECODE;
        } else if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr, "b64: help/version must be used alone\n");
            return 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "b64: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (text != NULL) {
            fprintf(stderr, "b64: only one text argument is accepted\n");
            return 1;
        } else {
            text = argv[i];
        }
    }

    if (mode == B64_MODE_NONE) {
        fprintf(stderr, "b64: -e/--encode or -d/--decode is required\n");
        return 1;
    }

    if (mode == B64_MODE_DECODE) {
        const char *input;
        char *stdin_text = NULL;
        void *decoded;
        size_t decoded_size = 0;

        if (text) {
            input = text;
        } else {
            if (b64_cli_read_stdin(&stdin_data, &stdin_size)) {
                fprintf(stderr, "b64: cannot read stdin\n");
                return 1;
            }
            stdin_text = (char *)malloc(stdin_size + 1);
            if (!stdin_text) {
                free(stdin_data);
                return 1;
            }
            memcpy(stdin_text, stdin_data, stdin_size);
            stdin_text[stdin_size] = '\0';
            free(stdin_data);
            input = stdin_text;
        }

        decoded = kc_b64_decode(input, &decoded_size);
        free(stdin_text);
        if (!decoded) {
            fprintf(stderr, "b64: invalid base64 input\n");
            return 1;
        }
        if (decoded_size > 0 &&
            fwrite(decoded, 1, decoded_size, stdout) != decoded_size)
        {
            kc_b64_free(decoded);
            fprintf(stderr, "b64: cannot write output\n");
            return 1;
        }
        kc_b64_free(decoded);
        return 0;
    }

    if (text) {
        char *encoded = kc_b64_encode(text, strlen(text));

        if (!encoded) {
            fprintf(stderr, "b64: encoding failed\n");
            return 1;
        }
        fputs(encoded, stdout);
        kc_b64_free(encoded);
        return 0;
    }

    if (b64_cli_read_stdin(&stdin_data, &stdin_size)) {
        fprintf(stderr, "b64: cannot read stdin\n");
        return 1;
    }
    {
        char *encoded = kc_b64_encode(stdin_data, stdin_size);

        free(stdin_data);
        if (!encoded) {
            fprintf(stderr, "b64: encoding failed\n");
            return 1;
        }
        fputs(encoded, stdout);
        kc_b64_free(encoded);
    }
    return 0;
}
