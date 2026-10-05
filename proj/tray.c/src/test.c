/**
 * test.c - tray CLI contract tests.
 * Summary: Validates public tray argument parsing and metadata commands.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "tray"
#endif

static int total = 0;
static int failed = 0;

/**
 * Runs one shell command and returns whether it succeeded.
 * @param command Command line.
 * @return Nonzero on success.
 */
static int run_ok(const char *command)
{
    return system(command) == 0;
}

/**
 * Prints one test result.
 * @param name Test name.
 * @param detail Behavior under test.
 * @param ok Nonzero on success.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    total++;
    if (!ok) failed++;
    printf("[%d/7] [%s] %s: %s\n", total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs tray CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    char command[1024];

    snprintf(command, sizeof(command), "\"%s\" --help > /dev/null 2>&1",
        KC_CLI_PATH);
    result("help", "--help succeeds", run_ok(command));

    snprintf(command, sizeof(command), "\"%s\" --version > /dev/null 2>&1",
        KC_CLI_PATH);
    result("version", "--version succeeds", run_ok(command));

    snprintf(command, sizeof(command), "\"%s\" --bad > /dev/null 2>&1",
        KC_CLI_PATH);
    result("unknown", "unknown options fail", !run_ok(command));

    snprintf(command, sizeof(command), "\"%s\" --icon > /dev/null 2>&1",
        KC_CLI_PATH);
    result("icon", "--icon requires a value", !run_ok(command));

    snprintf(command, sizeof(command), "\"%s\" --tooltip > /dev/null 2>&1",
        KC_CLI_PATH);
    result("tooltip", "--tooltip requires a value", !run_ok(command));

    snprintf(command, sizeof(command), "\"%s\" --item label > /dev/null 2>&1",
        KC_CLI_PATH);
    result("item", "--item requires text and command", !run_ok(command));

    snprintf(command, sizeof(command), "\"%s\" --quit > /dev/null 2>&1",
        KC_CLI_PATH);
    result("quit", "--quit requires a label", !run_ok(command));

    printf("\n%d passed, %d failed\n", total - failed, failed);
    return failed ? 1 : 0;
}
