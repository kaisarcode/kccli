/**
 * test.c - wch CLI contract tests.
 * Summary: Validates watcher administration and command-line behavior.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "wch"
#endif

static int test_total = 0;
static int test_failed = 0;

/**
 * Executes the CLI with output discarded.
 * @param args Arguments appended after the executable.
 * @return Child process status from system().
 */
static int run_cli(const char *args)
{
    char command[4096];

#ifdef _WIN32
    snprintf(
        command,
        sizeof(command),
        "\"\"%s\" %s > NUL 2>&1\"",
        KC_CLI_PATH,
        args ? args : ""
    );
#else
    snprintf(
        command,
        sizeof(command),
        "\"%s\" %s > /dev/null 2>&1",
        KC_CLI_PATH,
        args ? args : ""
    );
#endif
    return system(command);
}

/**
 * Executes the CLI and captures standard output.
 * @param args Arguments appended after the executable.
 * @param buffer Output buffer.
 * @param buffer_size Output buffer capacity.
 * @return Nonzero on success.
 */
static int capture_output(
    const char *args,
    char *buffer,
    size_t buffer_size
)
{
    const char *output_path = "wch-cli-output.txt";
    char command[4096];
    FILE *file;
    size_t size;

#ifdef _WIN32
    snprintf(
        command,
        sizeof(command),
        "\"\"%s\" %s > %s 2>NUL\"",
        KC_CLI_PATH,
        args ? args : "",
        output_path
    );
#else
    snprintf(
        command,
        sizeof(command),
        "\"%s\" %s > %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        output_path
    );
#endif
    if (system(command) != 0) {
        remove(output_path);
        return 0;
    }
    file = fopen(output_path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, buffer_size - 1U, file);
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
    printf(
        "[%d] [%s] %s: %s\n",
        test_total,
        ok ? "PASS" : "FAIL",
        name,
        detail
    );
}

#ifndef _WIN32
/**
 * Configures an isolated watcher runtime directory.
 * @return Nonzero on success.
 */
static int configure_runtime(void)
{
    return setenv("KC_WCH_DIR", "wch-cli-runtime", 1) == 0;
}

/**
 * Removes the isolated watcher runtime directory.
 * @return None.
 */
static void clear_runtime(void)
{
    (void)system("rm -rf wch-cli-runtime");
}
#endif

/**
 * Runs all wch CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    char output[4096];

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing", "missing watcher name fails", run_cli("") != 0);
    result(
        "create command",
        "creation without -c fails",
        run_cli("sample .") != 0
    );
    result(
        "unknown",
        "unknown option fails",
        run_cli("sample --unknown") != 0
    );
    result(
        "bad name",
        "invalid watcher name fails",
        run_cli("bad/name -d") != 0
    );

#ifndef _WIN32
    if (!configure_runtime()) {
        result("runtime", "isolated runtime is configured", 0);
    } else {
        clear_runtime();
        result(
            "create",
            "watcher creation succeeds",
            run_cli("sample . -c true") == 0
        );
        result(
            "list",
            "list emits tab-separated watcher fields",
            capture_output("-l sample", output, sizeof(output)) &&
            strncmp(output, "sample\t.\ttrue\t0\t", 16) == 0
        );
        result(
            "delete",
            "watcher deletion succeeds",
            run_cli("sample -d") == 0
        );
        clear_runtime();
    }
#else
    result(
        "list",
        "listing succeeds with no matching watcher",
        capture_output("-l wch-cli-missing", output, sizeof(output))
    );
    result(
        "delete",
        "deleting an absent watcher succeeds",
        run_cli("wch-cli-missing -d") == 0
    );
#endif

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
