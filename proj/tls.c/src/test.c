/**
 * test.c - tls CLI contract tests.
 * Summary: Exercises client/server command parsing and metadata behavior.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "tls"
#endif

#ifdef _WIN32
#define DEV_NULL "NUL"
#else
#define DEV_NULL "/dev/null"
#endif

#define TLS_TEST_COUNT 14

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
        TLS_TEST_COUNT,
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
    result("missing command", "empty invocation fails", run("") != 0);
    result(
        "unknown command",
        "unknown command fails",
        run("unknown") != 0
    );
    result(
        "client arguments",
        "client requires host and port",
        run("client localhost") != 0
    );
    result(
        "client port",
        "client rejects invalid port",
        run("client localhost invalid") != 0
    );
    result(
        "client options",
        "client rejects server-only options",
        run("client localhost 443 --cert server.pem") != 0
    );
    result(
        "server credentials",
        "server requires certificate and key",
        run("server 443") != 0
    );
    result(
        "server port",
        "server rejects invalid port",
        run("server invalid --cert server.pem --key server.key") != 0
    );
    result(
        "server options",
        "server rejects client-only options",
        run("server 443 --ca ca.pem --cert server.pem --key server.key") != 0
    );
    result(
        "missing option value",
        "value options require a following argument",
        run("client localhost 443 --ca") != 0
    );
    result(
        "unknown option",
        "unknown options fail",
        run("client localhost 443 --unknown") != 0
    );
    printf(
        "\n%d passed, %d failed\n",
        TLS_TEST_COUNT - test_failures,
        test_failures
    );
    return test_failures == 0 ? 0 : 1;
}
