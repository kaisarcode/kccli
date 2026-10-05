/**
 * test.c - min CLI contract tests.
 * Summary: Validates minification syntax and input precedence.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "min"
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
 * Captures one CLI invocation and compares stdout exactly.
 * @param args Arguments appended after executable.
 * @param stdin_text Optional text piped to stdin.
 * @param expected Expected stdout.
 * @return Nonzero on success.
 */
static int output_equals(
    const char *args,
    const char *stdin_text,
    const char *expected
)
{
    const char *input_path = "min-cli-stdin.txt";
    const char *output_path = "min-cli-output.txt";
    char command[4096];
    char buffer[4096];
    FILE *file;
    size_t size;

    if (stdin_text && !write_text_file(input_path, stdin_text)) return 0;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>NUL",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< min-cli-stdin.txt " : "",
        output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< min-cli-stdin.txt " : "",
        output_path);
#endif
    if (system(command) != 0) {
        if (stdin_text) remove(input_path);
        remove(output_path);
        return 0;
    }

    file = fopen(output_path, "rb");
    if (!file) {
        if (stdin_text) remove(input_path);
        return 0;
    }
    size = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    if (stdin_text) remove(input_path);
    remove(output_path);
    buffer[size] = '\0';
    return strcmp(buffer, expected) == 0;
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
 * Runs all min CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    const char *file_path = "min-cli-file.txt";
    int fixture_ok;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("default txt", "text argument defaults to txt",
        output_equals("\"fdfsdfsd sdfsd     ff sd              fsdfd\"", NULL,
            "fdfsdfsd sdfsd ff sd fsdfd"));
    result("stdin", "stdin defaults to txt",
        output_equals(NULL, "  hello   world\n\nfoo\tbar  ",
            "hello world foo bar"));

    fixture_ok = write_text_file(file_path, "file      input");
    result("input file", "-i reads file when higher-priority input is absent",
        fixture_ok && output_equals("-i min-cli-file.txt", NULL,
            "file input"));
    result("argument priority", "argument takes priority over -i",
        fixture_ok && output_equals(
            "\"argument    input\" -i min-cli-file.txt", NULL,
            "argument input"));
    result("stdin priority", "stdin takes priority over argument and -i",
        fixture_ok && output_equals(
            "\"argument input\" -i min-cli-file.txt",
            "stdin      input", "stdin input"));
    remove(file_path);

    result("css", "CSS argument minification succeeds",
        output_equals("-t css \"/* hi */ body{}\"", NULL, "body{}"));
    result("js", "JavaScript argument minification succeeds",
        output_equals("-t js \"const x = 1; // comment\"", NULL,
            "const x = 1;"));
    result("html", "HTML argument minification succeeds",
        output_equals("-t html \"<!-- c --><p>hi</p>\"", NULL, "<p>hi</p>"));
    result("bad type", "unknown type fails", run_cli("-t xml \"text\"") != 0);
    result("unknown", "unknown option fails", run_cli("--unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
