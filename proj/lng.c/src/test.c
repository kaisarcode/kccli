/**
 * test.c - lng CLI contract tests.
 * Summary: Validates language detection argument parsing and execution.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "lng"
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
 * Runs all lng CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("detect", "text detection succeeds",
        run_cli("\"Hello world\"") == 0);
    result("threshold", "valid threshold succeeds",
        run_cli("\"Hello world\" --threshold 0.1") == 0);
    result("limit", "valid limit succeeds",
        run_cli("\"Hello world\" --limit 3") == 0);
    result("bad threshold", "invalid threshold fails",
        run_cli("\"Hello world\" --threshold 2") != 0);
    result("bad limit", "zero limit fails",
        run_cli("\"Hello world\" --limit 0") != 0);
    result("unknown", "unknown option fails",
        run_cli("\"Hello world\" --unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
