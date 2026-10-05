/**
 * test.c - b64 CLI contract tests.
 * Summary: Validates Base64 argument and stdin behavior.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "b64"
#endif

static int test_total = 0;
static int test_failed = 0;

/**
 * Executes one shell command and captures stdout.
 * @param args CLI arguments.
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
    const char *output_path = "b64-cli-output.txt";
    char command[4096];
    char buffer[4096];
    FILE *file;
    size_t size;
    int status;

#ifdef _WIN32
    if (stdin_text) {
        snprintf(command, sizeof(command),
            "echo|set /p=\"%s\" | \"%s\" %s > %s 2>NUL",
            stdin_text, KC_CLI_PATH, args ? args : "", output_path);
    } else {
        snprintf(command, sizeof(command),
            "\"%s\" %s > %s 2>NUL",
            KC_CLI_PATH, args ? args : "", output_path);
    }
#else
    if (stdin_text) {
        snprintf(command, sizeof(command),
            "printf '%%s' '%s' | \"%s\" %s > %s 2>/dev/null",
            stdin_text, KC_CLI_PATH, args ? args : "", output_path);
    } else {
        snprintf(command, sizeof(command),
            "\"%s\" %s > %s 2>/dev/null",
            KC_CLI_PATH, args ? args : "", output_path);
    }
#endif

    status = system(command);
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
    printf("[%d/8] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs all b64 CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("encode arg", "text argument encodes",
        output_equals("\"hello world\"", NULL, "aGVsbG8gd29ybGQ="));
    result("encode stdin", "stdin encodes when argument is absent",
        output_equals("", "hello world", "aGVsbG8gd29ybGQ="));
    result("argument priority", "argument takes priority over stdin",
        output_equals("\"hello\"", "ignored", "aGVsbG8="));
    result("decode arg", "Base64 argument decodes",
        output_equals("-d aGVsbG8=", NULL, "hello"));
    result("decode stdin", "Base64 stdin decodes",
        output_equals("-d", "aGVsbG8=", "hello"));
    result("invalid", "invalid Base64 fails", run_cli("-d invalid") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
