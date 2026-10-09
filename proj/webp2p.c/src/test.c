/**
 * test.c - webp2p CLI contract tests.
 * Summary: Checks the public command-line surface without external services.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "webp2p"
#endif

static int test_total;
static int test_current;
static int test_failures;

static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf("[%d/%d] [%s] %s: %s\n", test_current, test_total,
        ok ? "PASS" : "FAIL", name, detail);
}

static int run_cli(const char *args)
{
    char command[8192];
    int status;

    if ((size_t)snprintf(command, sizeof(command),
        "\"%s\" %s > /dev/null 2>&1", KC_CLI_PATH,
        args ? args : "") >= sizeof(command))
        return -1;
    status = system(command);
    if (status == -1 || !WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
}

int main(void)
{
    test_total = 12;
    result("help", "prints command help", run_cli("--help") == 0);
    result("version", "prints build version", run_cli("--version") == 0);
    result("command", "rejects unknown command", run_cli("unknown") != 0);
    result("pub port", "requires publisher local port",
        run_cli("pub demo@https://example.invalid/") != 0);
    result("con port", "requires consumer local port",
        run_cli("con demo@https://example.invalid/") != 0);
    result("index URL", "requires an HTTP index URL",
        run_cli("pub demo@index.example -p 8080") != 0);
    result("transport", "rejects unsupported local transport",
        run_cli("pub demo@https://example.invalid/ -p 8080 -t udp") != 0);
    result("consumer pass", "rejects publisher password on consumer",
        run_cli("con demo@https://example.invalid/ -p 8080 --pass secret") != 0);
    result("turn pair", "requires both TURN credentials",
        run_cli("pub demo@https://example.invalid/ -p 8080 --turn turn:x --turn-user u") != 0);
    result("turn URL", "requires TURN URL for credentials",
        run_cli("pub demo@https://example.invalid/ -p 8080 --turn-user u --turn-pass p") != 0);
    result("force turn", "requires TURN URL when relay is forced",
        run_cli("pub demo@https://example.invalid/ -p 8080 --force-turn 1") != 0);
    result("positional", "rejects extra positional arguments",
        run_cli("pub demo@https://example.invalid/ extra -p 8080") != 0);

    printf("\n%d passed, %d failed\n",
        test_total - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
