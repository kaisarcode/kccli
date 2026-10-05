/**
 * test.c - tpl CLI contract tests.
 * Summary: Validates template rendering argument parsing and execution.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "tpl"
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
 * Verifies one rendered output.
 * @return Nonzero on success.
 */
static int render_output_ok(void)
{
    FILE *file;
    char command[4096];
    char buffer[256];
    size_t size;
    int ok;

    file = fopen("tpl-cli-input.tpl", "wb");
    if (!file) return 0;
    fputs("Hello {{ name }}!", file);
    fclose(file);

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"%s\" tpl-cli-input.tpl --var name Kaisar > tpl-cli-output.txt 2>NUL",
        KC_CLI_PATH);
#else
    snprintf(command, sizeof(command),
        "\"%s\" tpl-cli-input.tpl --var name Kaisar > tpl-cli-output.txt 2>/dev/null",
        KC_CLI_PATH);
#endif
    if (system(command) != 0) {
        remove("tpl-cli-input.tpl");
        remove("tpl-cli-output.txt");
        return 0;
    }

    file = fopen("tpl-cli-output.txt", "rb");
    if (!file) {
        remove("tpl-cli-input.tpl");
        return 0;
    }
    size = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[size] = '\0';
    ok = strcmp(buffer, "Hello Kaisar!") == 0;
    remove("tpl-cli-input.tpl");
    remove("tpl-cli-output.txt");
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
    printf("[%d/7] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs all tpl CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing", "missing template path fails", run_cli("") != 0);
    result("root", "--root requires a value", run_cli("x --root") != 0);
    result("var", "--var requires key and value", run_cli("x --var key") != 0);
    result("unknown", "unknown option fails", run_cli("x --unknown") != 0);
    result("render", "renders variables to stdout", render_output_ok());

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
