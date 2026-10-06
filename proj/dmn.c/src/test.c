/**
 * test.c - dmn CLI contract tests.
 * Summary: Validates managed daemon lifecycle and command-line behavior.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "dmn"
#endif

#define DMN_TEST_COUNT 9

static int test_current;
static int test_failures;

/**
 * Print one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Whether the test passed.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf(
        "[%d/%d] [%s] %s: %s\n",
        test_current,
        DMN_TEST_COUNT,
        ok ? "PASS" : "FAIL",
        name,
        detail
    );
}

/**
 * Execute the CLI with output discarded.
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
 * Execute the CLI and capture standard output.
 * @param args Arguments appended after the executable.
 * @param buffer Output buffer.
 * @param capacity Buffer capacity.
 * @return Nonzero on success.
 */
static int capture_output(
    const char *args,
    char *buffer,
    size_t capacity
)
{
    const char *path = "dmn-cli-output.txt";
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
        path
    );
#else
    snprintf(
        command,
        sizeof(command),
        "\"%s\" %s > %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        path
    );
#endif
    if (system(command) != 0) {
        (void)remove(path);
        return 0;
    }
    file = fopen(path, "r");
    if (!file) return 0;
    size = fread(buffer, 1, capacity - 1U, file);
    fclose(file);
    (void)remove(path);
    buffer[size] = '\0';
    return 1;
}

/**
 * Remove the isolated runtime directory.
 * @return None.
 */
static void clear_runtime(void)
{
#ifdef _WIN32
    (void)system("rmdir /S /Q dmn-cli-runtime >NUL 2>&1");
#else
    (void)system("rm -rf dmn-cli-runtime");
#endif
}

/**
 * Configure an isolated runtime directory.
 * @return Nonzero on success.
 */
static int configure_runtime(void)
{
#ifdef _WIN32
    return _putenv_s("KC_DMN_DIR", "dmn-cli-runtime") == 0;
#else
    return setenv("KC_DMN_DIR", "dmn-cli-runtime", 1) == 0;
#endif
}

/**
 * Run all dmn CLI contract tests.
 * @return Process status.
 */
int main(void)
{
    char output[1024];
    const char *command;
    const char *expected;
    int runtime_ok;
    int create_ok;
    int list_ok;
    int update_ok;
    int delete_ok;

    test_current = 0;
    test_failures = 0;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing", "missing daemon id fails", run_cli("") != 0);
    result(
        "unknown",
        "unknown option fails",
        run_cli("sample cat --unknown") != 0
    );
    result(
        "bad id",
        "invalid daemon id fails",
        run_cli("bad/name -d") != 0
    );

    runtime_ok = configure_runtime();
    clear_runtime();
#ifdef _WIN32
    command = "more";
    expected = "sample\tmore\n";
#else
    command = "cat";
    expected = "sample\tcat\n";
#endif

    create_ok = runtime_ok && run_cli(
#ifdef _WIN32
        "sample more"
#else
        "sample cat"
#endif
    ) == 0;
    result(
        "create",
        "registers a resident daemon",
        create_ok
    );

    output[0] = '\0';
    list_ok = create_ok && capture_output(
        "-l sample",
        output,
        sizeof(output)
    ) && strcmp(output, expected) == 0;
    result(
        "list",
        "list emits id and command",
        list_ok
    );

    update_ok = create_ok && run_cli(
#ifdef _WIN32
        "sample more"
#else
        "sample cat"
#endif
    ) == 0;
    result(
        "update",
        "reusing an id updates the daemon",
        update_ok
    );

    delete_ok = run_cli("sample -d") == 0;
    output[0] = '\0';
    if (delete_ok) {
        delete_ok = capture_output(
            "-l sample",
            output,
            sizeof(output)
        ) && output[0] == '\0';
    }
    result(
        "delete",
        "removes the resident daemon",
        delete_ok
    );

    (void)command;
    clear_runtime();
    printf(
        "\n%d passed, %d failed\n",
        DMN_TEST_COUNT - test_failures,
        test_failures
    );
    return test_failures ? 1 : 0;
}
