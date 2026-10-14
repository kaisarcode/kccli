/**
 * test.c - pow CLI contract tests.
 * Summary: Exercises basic challenge, solve, verify, help, and version behavior.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "pow"
#endif

#ifdef _WIN32
#define DEV_NULL "NUL"
#else
#define DEV_NULL "/dev/null"
#endif

/**
 * Runs a CLI command and returns its exit status.
 * @param args Command arguments.
 * @return Exit status.
 */
static int run(const char *args)
{
    char command[4096];
    int n = snprintf(command, sizeof(command), "%s %s", KC_CLI_PATH, args);
    if (n < 0 || (size_t)n >= sizeof(command)) return 1;
    return system(command);
}

/**
 * Test entry point.
 * @return Exit status.
 */
int main(void)
{
    int fail = 0;

    if (run("--help > " DEV_NULL) != 0) fail++;
    if (run("--version > " DEV_NULL) != 0) fail++;
    if (run("challenge 0 > " DEV_NULL " 2>&1") == 0) fail++;
    if (run("challenge 4 > pow-test-challenge.bin") != 0) fail++;
    if (run("solve -i pow-test-challenge.bin > pow-test-solution.bin") != 0) fail++;
    if (run("verify -c pow-test-challenge.bin -i pow-test-solution.bin") != 0) fail++;
    if (run("verify -i pow-test-solution.bin > " DEV_NULL " 2>&1") == 0) fail++;

    remove("pow-test-solution.bin");
    remove("pow-test-challenge.bin");

    printf("%d passed, %d failed\n", 7 - fail, fail);
    return fail ? 1 : 0;
}
