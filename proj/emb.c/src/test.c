/**
 * test.c - emb CLI contract tests.
 * Summary: Validates embedding argument parsing and execution.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "emb"
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
    snprintf(command, sizeof(command), "\"%s\" %s > NUL 2>&1",
        KC_CLI_PATH, args ? args : "");
#else
    snprintf(command, sizeof(command), "\"%s\" %s > /dev/null 2>&1",
        KC_CLI_PATH, args ? args : "");
#endif
    return system(command);
}

/**
 * Verifies that embedding output is comma-separated without a header.
 * @return Nonzero on success.
 */
static int csv_output_ok(void)
{
    const char *path = "emb-cli-output.txt";
    char command[4096];
    char buffer[8192];
    FILE *file;
    size_t size;
    int ok;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"%s\" \"Hello world\" > %s 2>NUL", KC_CLI_PATH, path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" \"Hello world\" > %s 2>/dev/null", KC_CLI_PATH, path);
#endif
    if (system(command) != 0) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    remove(path);
    buffer[size] = '\0';
    ok = size > 0 && strchr(buffer, ',') != NULL && strchr(buffer, '\t') == NULL;
    if (strncmp(buffer, "code,", 5) == 0 || strncmp(buffer, "value,", 6) == 0)
        ok = 0;
    return ok;
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
 * Runs all emb CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("dimension", "--dimension succeeds", run_cli("--dimension") == 0);
    result("embed", "text embedding succeeds", run_cli("\"Hello world\"") == 0);
    result("csv", "embedding output is headerless CSV", csv_output_ok());
    result("unknown", "unknown option fails",
        run_cli("\"Hello world\" --unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
