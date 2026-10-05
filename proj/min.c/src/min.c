/**
 * min.c - Asset minification command-line interface.
 * Summary: Minifies CSS, JavaScript, HTML, or text with libmin.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "libmin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Prints public CLI usage.
 * @param name Program executable name.
 * @return None.
 */
static void min_cli_usage(const char *name)
{
    printf("Usage: %s [text] [options]\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  -t, --type <type>  css, js, html, or txt (default: txt)\n");
    printf("  -h, --help         Show this help\n");
    printf("  -v, --version      Show version\n");
}

/**
 * Reads all bytes from standard input as a NUL-terminated string.
 * @param out Receives allocated string.
 * @return Zero on success, nonzero on failure.
 */
static int min_cli_read_stdin(char **out)
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
 * Dispatches minification by public type name.
 * @param type Type name.
 * @param input Input text.
 * @return Owned minified output, or NULL on failure.
 */
static char *min_cli_minify(const char *type, const char *input)
{
    if (strcmp(type, "css") == 0) return kc_min_css(input);
    if (strcmp(type, "js") == 0) return kc_min_js(input);
    if (strcmp(type, "html") == 0) return kc_min_html(input);
    if (strcmp(type, "txt") == 0) return kc_min_txt(input);
    return NULL;
}

/**
 * Runs the min CLI.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    const char *type = "txt";
    const char *text = NULL;
    const char *input;
    char *stdin_input = NULL;
    char *output;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0))
    {
        min_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0))
    {
        printf("%llu\n", (unsigned long long)kc_min_version());
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--type") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "min: --type requires a value\n");
                return 1;
            }
            type = argv[i];
        } else if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr, "min: help/version must be used alone\n");
            return 1;
        } else if (argv[i][0] == '-' && strcmp(argv[i], "-") != 0) {
            fprintf(stderr, "min: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (text != NULL) {
            fprintf(stderr, "min: only one text argument is accepted; quote spaces\n");
            return 1;
        } else {
            text = argv[i];
        }
    }

    if (strcmp(type, "css") != 0 && strcmp(type, "js") != 0 &&
        strcmp(type, "html") != 0 && strcmp(type, "txt") != 0)
    {
        fprintf(stderr, "min: type must be css, js, html, or txt\n");
        return 1;
    }

    if (text) {
        input = text;
    } else {
        if (min_cli_read_stdin(&stdin_input)) {
            fprintf(stderr, "min: cannot read stdin\n");
            return 1;
        }
        input = stdin_input;
    }

    output = min_cli_minify(type, input);
    free(stdin_input);
    if (!output) {
        fprintf(stderr, "min: minification failed\n");
        return 1;
    }

    fputs(output, stdout);
    kc_min_free(output);
    return 0;
}
