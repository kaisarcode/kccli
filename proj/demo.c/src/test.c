/**
 * test.c - demo CLI contract tests.
 * Summary: Validates greeting, help, version, and argument errors.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "demo"
#endif

static int test_total = 0;
static int test_failed = 0;

/**
 * Executes the CLI with output discarded.
 * @param args Arguments appended after executable.
 * @return Child process status from system().
 */
static int run_cli(const char *args)
{
    char command[4096];

#ifdef _WIN32
    snprintf(command, sizeof(command), "\"\"%s\" %s > NUL 2>&1\"",
        KC_CLI_PATH, args ? args : "");
#else
    snprintf(command, sizeof(command), "\"%s\" %s > /dev/null 2>&1",
        KC_CLI_PATH, args ? args : "");
#endif
    return system(command);
}

/**
 * Executes the CLI and captures stdout.
 * @param args Arguments appended after executable.
 * @param buffer Output buffer.
 * @param buffer_size Output buffer size.
 * @return Nonzero on success.
 */
static int capture_output(const char *args, char *buffer, size_t buffer_size)
{
    const char *output_path = "demo-cli-output.txt";
    char command[4096];
    FILE *file;
    size_t size;

#ifdef _WIN32
    snprintf(command, sizeof(command), "\"\"%s\" %s > %s 2>NUL\"",
        KC_CLI_PATH, args ? args : "", output_path);
#else
    snprintf(command, sizeof(command), "\"%s\" %s > %s 2>/dev/null",
        KC_CLI_PATH, args ? args : "", output_path);
#endif

    if (system(command) != 0) {
        remove(output_path);
        return 0;
    }

    file = fopen(output_path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, buffer_size - 1, file);
    fclose(file);
    remove(output_path);
    buffer[size] = '\0';
    return 1;
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
    printf("[%d/6] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs all demo CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    char output[256];

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("greet", "name prints greeting",
        capture_output("John", output, sizeof(output)) &&
        strcmp(output, "Hello John!\n") == 0);
    result("quoted", "quoted name with spaces succeeds",
        capture_output("\"John Doe\"", output, sizeof(output)) &&
        strcmp(output, "Hello John Doe!\n") == 0);
    result("missing", "missing name fails", run_cli(NULL) != 0);
    result("unknown", "unknown option fails", run_cli("--unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
