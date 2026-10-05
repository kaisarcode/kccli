/**
 * emb.c - Text embedding command-line interface.
 * Summary: Generates fixed-model text embeddings with libemb.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "libemb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Prints public CLI usage.
 * @param name Program executable name.
 * @return None.
 */
static void emb_cli_usage(const char *name)
{
    printf("Usage: %s [text] [options]\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  -d, --dimension  Print embedding dimension\n");
    printf("  -h, --help       Show this help\n");
    printf("  -v, --version    Show version\n");
}

/**
 * Reads all standard input into a NUL-terminated string.
 * @param out Receives allocated input string.
 * @return Zero on success, nonzero on failure.
 */
static int emb_cli_read_stdin(char **out)
{
    char *buffer = NULL;
    size_t size = 0;
    size_t capacity = 0;
    char chunk[4096];
    size_t count;

    *out = NULL;
    while ((count = fread(chunk, 1, sizeof(chunk), stdin)) > 0) {
        size_t required = size + count + 1;
        char *next;

        if (required > capacity) {
            size_t next_capacity = capacity ? capacity * 2 : 4096;

            while (next_capacity < required) next_capacity *= 2;
            next = (char *)realloc(buffer, next_capacity);
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
        buffer = (char *)malloc(1);
        if (!buffer) return 1;
    }
    buffer[size] = '\0';
    *out = buffer;
    return 0;
}

/**
 * Runs the emb CLI.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    const char *text = NULL;
    char *stdin_text = NULL;
    float *vector = NULL;
    size_t count = 0;
    int dimension = 0;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0))
    {
        emb_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0))
    {
        printf("%llu\n", (unsigned long long)kc_emb_version());
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-d") == 0 ||
        strcmp(argv[1], "--dimension") == 0))
    {
        printf("%zu\n", kc_emb_dimension());
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 ||
            strcmp(argv[i], "--dimension") == 0)
        {
            dimension = 1;
        } else if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr,
                "emb: -h/--help and -v/--version must be used alone\n");
            return 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "emb: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (text != NULL) {
            fprintf(stderr,
                "emb: only one text argument is accepted; quote spaces\n");
            return 1;
        } else {
            text = argv[i];
        }
    }

    if (dimension) {
        fprintf(stderr, "emb: --dimension must be used alone\n");
        return 1;
    }

    if (text == NULL) {
        if (emb_cli_read_stdin(&stdin_text)) {
            fprintf(stderr, "emb: cannot read stdin\n");
            return 1;
        }
        text = stdin_text;
    }

    if (kc_emb_embed(text, &vector, &count) != KC_EMB_OK) {
        fprintf(stderr, "emb: embedding failed\n");
        free(stdin_text);
        return 1;
    }

    for (size_t i = 0; i < count; i++) {
        if (i) putchar(',');
        printf("%.9g", (double)vector[i]);
    }
    putchar('\n');

    kc_emb_free(vector);
    free(stdin_text);
    return 0;
}
