/**
 * tpl.c - Template renderer command-line interface.
 * Summary: Renders one template with variables through libtpl.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "libtpl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Prints public CLI usage.
 * @param name Program executable name.
 * @return None.
 */
static void tpl_cli_usage(const char *name)
{
    printf("Usage: %s <template> [options]\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  -r, --root <path>       Include root (default: .)\n");
    printf("  -v, --var <key> <value> Set a template variable; repeatable\n");
    printf("  -h, --help              Show this help\n");
    printf("  -V, --version           Show version\n");
}

/**
 * Reads all bytes from one stream as a NUL-terminated string.
 * @param stream Input stream.
 * @param out Receives allocated string.
 * @return Zero on success, nonzero on failure.
 */
static int tpl_cli_read_stream(FILE *stream, char **out)
{
    char *buffer = NULL;
    size_t size = 0;
    size_t capacity = 0;
    char chunk[4096];
    size_t count;

    *out = NULL;
    while ((count = fread(chunk, 1, sizeof(chunk), stream)) > 0) {
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
    if (ferror(stream)) {
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
 * Reads one template source from a file or standard input.
 * @param path File path, or "-" for standard input.
 * @param out Receives allocated template source.
 * @return Zero on success, nonzero on failure.
 */
static int tpl_cli_read_source(const char *path, char **out)
{
    FILE *file;
    int status;

    if (strcmp(path, "-") == 0) return tpl_cli_read_stream(stdin, out);
    file = fopen(path, "rb");
    if (!file) return 1;
    status = tpl_cli_read_stream(file, out);
    fclose(file);
    return status;
}

/**
 * Runs the tpl CLI.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    const char *template_path = NULL;
    const char *root = NULL;
    kc_tpl_var_t *vars = NULL;
    size_t var_count = 0;
    char *source = NULL;
    char *output = NULL;
    kc_tpl_t *tpl = NULL;
    kc_tpl_options_t options;
    int result = 1;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0))
    {
        tpl_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-V") == 0 ||
        strcmp(argv[1], "--version") == 0))
    {
        printf("%llu\n", (unsigned long long)kc_tpl_version());
        return 0;
    }

    vars = (kc_tpl_var_t *)calloc((size_t)argc, sizeof(*vars));
    if (!vars) return 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--root") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "tpl: --root requires a path\n");
                goto done;
            }
            root = argv[i];
        } else if (strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--var") == 0)
        {
            if (i + 2 >= argc || !argv[i + 1][0]) {
                fprintf(stderr, "tpl: --var requires <key> <value>\n");
                goto done;
            }
            vars[var_count].key = argv[i + 1];
            vars[var_count].value = argv[i + 2];
            var_count++;
            i += 2;
        } else if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-V") == 0 ||
            strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr,
                "tpl: help and version options must be used alone\n");
            goto done;
        } else if (argv[i][0] == '-' && strcmp(argv[i], "-") != 0) {
            fprintf(stderr, "tpl: unknown option '%s'\n", argv[i]);
            goto done;
        } else if (template_path != NULL) {
            fprintf(stderr, "tpl: only one template path is accepted\n");
            goto done;
        } else {
            template_path = argv[i];
        }
    }

    if (!template_path) {
        fprintf(stderr, "tpl: missing template path\n");
        goto done;
    }
    if (tpl_cli_read_source(template_path, &source)) {
        fprintf(stderr, "tpl: cannot read template '%s'\n", template_path);
        goto done;
    }

    memset(&options, 0, sizeof(options));
    options.root = root;
    if (kc_tpl_open(&tpl, source, &options) != KC_TPL_OK) {
        fprintf(stderr, "tpl: %s\n", kc_tpl_error(tpl));
        goto done;
    }

    output = kc_tpl_render(tpl, vars, var_count);
    if (!output) {
        fprintf(stderr, "tpl: %s\n", kc_tpl_error(tpl));
        goto done;
    }
    if (fputs(output, stdout) == EOF) {
        fprintf(stderr, "tpl: cannot write output\n");
        goto done;
    }
    result = 0;

done:
    kc_tpl_free(output);
    kc_tpl_close(tpl);
    free(source);
    free(vars);
    return result;
}
