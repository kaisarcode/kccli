/**
 * mdp.c - Markdown parser command-line interface.
 * Summary: Renders Markdown HTML, body, or metadata with libmdp.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "libmdp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define MDP_MODE_HTML 0
#define MDP_MODE_BODY 1
#define MDP_MODE_META 2

/**
 * Prints public CLI usage.
 * @param name Program executable name.
 * @return None.
 */
static void mdp_cli_usage(const char *name)
{
    printf("Usage: %s [text] [options]\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  --body         Print Markdown body without frontmatter\n");
    printf("  --meta         Print raw frontmatter\n");
    printf("  -h, --help     Show this help\n");
    printf("  -v, --version  Show version\n");
}

/**
 * Reads all bytes from standard input as a NUL-terminated string.
 * @param out Receives allocated text.
 * @return Zero on success, nonzero on failure.
 */
static int mdp_cli_read_stdin(char **out)
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
 * Runs the mdp CLI.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    const char *text = NULL;
    char *stdin_text = NULL;
    const char *input;
    const char *output;
    kc_mdp_t *mdp = NULL;
    int mode = MDP_MODE_HTML;

#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
        fprintf(stderr, "mdp: cannot configure stdout\n");
        return 1;
    }
#endif

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0))
    {
        mdp_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0))
    {
        printf("%llu\n", (unsigned long long)kc_mdp_version());
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--body") == 0) {
            if (mode != MDP_MODE_HTML) {
                fprintf(stderr, "mdp: choose only one output mode\n");
                return 1;
            }
            mode = MDP_MODE_BODY;
        } else if (strcmp(argv[i], "--meta") == 0) {
            if (mode != MDP_MODE_HTML) {
                fprintf(stderr, "mdp: choose only one output mode\n");
                return 1;
            }
            mode = MDP_MODE_META;
        } else if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr, "mdp: help/version must be used alone\n");
            return 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "mdp: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (text != NULL) {
            fprintf(stderr, "mdp: only one text argument is accepted\n");
            return 1;
        } else {
            text = argv[i];
        }
    }

    if (text) {
        input = text;
    } else {
        if (mdp_cli_read_stdin(&stdin_text)) {
            fprintf(stderr, "mdp: cannot read stdin\n");
            return 1;
        }
        input = stdin_text;
    }

    if (kc_mdp_open(&mdp, input) != KC_MDP_OK) {
        free(stdin_text);
        fprintf(stderr, "mdp: cannot parse markdown\n");
        return 1;
    }
    free(stdin_text);

    if (mode == MDP_MODE_BODY) {
        output = kc_mdp_body(mdp);
    } else if (mode == MDP_MODE_META) {
        output = kc_mdp_meta(mdp);
    } else {
        output = kc_mdp_html(mdp);
    }

    if (!output) {
        kc_mdp_close(mdp);
        fprintf(stderr, "mdp: output failed\n");
        return 1;
    }

    fputs(output, stdout);
    kc_mdp_close(mdp);
    return 0;
}
