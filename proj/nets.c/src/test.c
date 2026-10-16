/**
 * test.c - nets CLI contract tests.
 * Summary: Exercises endpoint argument parsing and metadata behavior.
 *
 * Author: KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "nets"
#endif

#ifdef _WIN32
#define DEV_NULL "NUL"
#else
#define DEV_NULL "/dev/null"
#endif

#define NETS_TEST_COUNT 12

static int test_current;
static int test_failures;

/**
 * Prints one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Nonzero when the test passed.
 * @return void.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf(
        "[%d/%d] [%s] %s: %s\n",
        test_current,
        NETS_TEST_COUNT,
        ok ? "PASS" : "FAIL",
        name,
        detail
    );
}

/**
 * Runs the CLI with one argument string.
 * @param args Command arguments.
 * @return Shell exit status.
 */
static int run(const char *args)
{
    char command[4096];
    int count;

    count = snprintf(
        command,
        sizeof(command),
        "%s %s > " DEV_NULL " 2> " DEV_NULL,
        KC_CLI_PATH,
        args
    );
    if (count < 0 || (size_t)count >= sizeof(command)) return 1;
    return system(command);
}

/**
 * Runs all CLI contract tests.
 * @return Zero when every test passes.
 */
int main(void)
{
    result("help", "long help succeeds", run("--help") == 0);
    result("short help", "short help succeeds", run("-h") == 0);
    result("version", "long version succeeds", run("--version") == 0);
    result("short version", "short version succeeds", run("-v") == 0);
    result("missing endpoint", "empty invocation fails", run("") != 0);
    result("missing port", "host without port fails", run("localhost") != 0);
    result("zero port", "port zero fails", run("localhost 0") != 0);
    result(
        "invalid port",
        "non-numeric port fails",
        run("localhost invalid") != 0
    );
    result(
        "invalid transport",
        "transport requires tcp or udp",
        run("localhost 9 --transport tls") != 0
    );
    result(
        "transport value",
        "transport requires a following value",
        run("localhost 9 --transport") != 0
    );
    result(
        "removed tls option",
        "TLS option is not accepted",
        run("localhost 9 --tls") != 0
    );
    result(
        "extra positional",
        "extra endpoint arguments fail",
        run("localhost 9 extra") != 0
    );
    printf(
        "\n%d passed, %d failed\n",
        NETS_TEST_COUNT - test_failures,
        test_failures
    );
    return test_failures == 0 ? 0 : 1;
}
