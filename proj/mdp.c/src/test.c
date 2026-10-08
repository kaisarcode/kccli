/**
 * test.c - mdp CLI contract tests.
 * Summary: Validates Markdown rendering, views, and input precedence.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "mdp"
#endif

static int test_total = 0;
static int test_failed = 0;

/**
 * Writes one text fixture file.
 * @param path Output file path.
 * @param text Text to write.
 * @return Nonzero on success.
 */
static int write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "wb");
    size_t size = strlen(text);
    int ok;

    if (!file) return 0;
    ok = fwrite(text, 1, size, file) == size;
    fclose(file);
    return ok;
}

/**
 * Executes the CLI and compares captured stdout exactly.
 * @param args CLI arguments.
 * @param stdin_text Optional stdin content.
 * @param expected Expected stdout.
 * @return Nonzero on success.
 */
static int output_equals(
    const char *args,
    const char *stdin_text,
    const char *expected
)
{
    const char *input_path = "mdp-cli-input.txt";
    const char *output_path = "mdp-cli-output.txt";
    char command[4096];
    char buffer[8192];
    FILE *file;
    size_t size;
    int status;

    if (stdin_text) {
        if (!write_text_file(input_path, stdin_text)) return 0;
    }

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>NUL",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< mdp-cli-input.txt " : "",
        output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< mdp-cli-input.txt " : "",
        output_path);
#endif

    status = system(command);
    if (stdin_text) remove(input_path);
    if (status != 0) {
        remove(output_path);
        return 0;
    }

    file = fopen(output_path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    remove(output_path);
    buffer[size] = '\0';
    return strcmp(buffer, expected) == 0;
}

/**
 * Executes the CLI with output discarded.
 * @param args Arguments appended after executable.
 * @return Child process status from system().
 */
static int run_cli(const char *args)
{
    char command[4096];

#ifdef _WIN32
    snprintf(command, sizeof(command), "\"%s\" %s > NUL 2>&1",
        KC_CLI_PATH, args ? args : "");
#else
    snprintf(command, sizeof(command), "\"%s\" %s > /dev/null 2>&1",
        KC_CLI_PATH, args ? args : "");
#endif
    return system(command);
}

/**
 * Prints one contract-test result.
 * @param name Test name.
 * @param detail Behavior under test.
 * @param ok Nonzero on success.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_total++;
    if (!ok) test_failed++;
    printf("[%d/12] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs all mdp CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    const char *document = "---\ntitle: Home\n---\n# Hello";
    const char *file_path = "mdp-cli-file.md";
    int fixture_ok;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("html arg", "positional text renders HTML by default",
        output_equals("Hello", NULL, "<p>Hello</p>\n"));
    result("html stdin", "stdin renders HTML when argument is absent",
        output_equals("", "# Hello", "<h1>Hello</h1>\n"));
    result("ansi", "--ansi renders terminal output",
        output_equals("--ansi", "# Hello",
            "\x1b[36m\x1b[1mHello\x1b[22m\x1b[39m\n\n"));

    fixture_ok = write_text_file(file_path, "# File");
    result("input file", "-i reads file when higher-priority input is absent",
        fixture_ok && output_equals("-i mdp-cli-file.md", NULL,
            "<h1>File</h1>\n"));
    result("stdin priority", "stdin takes priority over -i",
        fixture_ok && output_equals("-i mdp-cli-file.md", "# Stdin",
            "<h1>Stdin</h1>\n"));
    result("argument priority", "argument takes priority over stdin and -i",
        fixture_ok && output_equals("-i mdp-cli-file.md Hello", "# Stdin",
            "<p>Hello</p>\n"));
    remove(file_path);

    result("body", "--body returns Markdown body",
        output_equals("--body", document, "# Hello"));
    result("meta", "--meta returns raw frontmatter",
        output_equals("--meta", document, "title: Home"));
    result("exclusive", "output modes are mutually exclusive",
        run_cli("--ansi --body Hello") != 0 &&
        run_cli("--body --meta Hello") != 0);
    result("unknown", "unknown option fails",
        run_cli("--unknown Hello") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
