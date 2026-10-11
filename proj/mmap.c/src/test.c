/**
 * test.c - mmap CLI contract tests.
 * Summary: Validates persistent binary value CLI behavior.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "mmap"
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
 * Executes get and captures its binary stdout.
 * @param path Backing value path.
 * @return Nonzero when output exactly matches the test payload.
 */
static int get_matches(const char *path)
{
    static const unsigned char expected[] = {0x00, 0x41, 0xff, 0x42};
    unsigned char data[sizeof(expected) + 1];
    char command[4096];
    FILE *file;
    size_t size;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"%s\" get %s > mmap-cli-output.bin 2>NUL", KC_CLI_PATH, path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" get %s > mmap-cli-output.bin 2>/dev/null", KC_CLI_PATH, path);
#endif
    if (system(command) != 0) return 0;
    file = fopen("mmap-cli-output.bin", "rb");
    if (!file) return 0;
    size = fread(data, 1, sizeof(data), file);
    fclose(file);
    remove("mmap-cli-output.bin");
    return size == sizeof(expected) && memcmp(data, expected, sizeof(expected)) == 0;
}

/**
 * Creates the binary input fixture.
 * @return Nonzero on success.
 */
static int create_input(void)
{
    static const unsigned char data[] = {0x00, 0x41, 0xff, 0x42};
    FILE *file = fopen("mmap-cli-input.bin", "wb");

    if (!file) return 0;
    if (fwrite(data, 1, sizeof(data), file) != sizeof(data)) {
        fclose(file);
        remove("mmap-cli-input.bin");
        return 0;
    }
    return fclose(file) == 0;
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
 * Runs all mmap CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    remove("mmap-cli-value.bin");
    remove("mmap-cli-input.bin");
    remove("mmap-cli-output.bin");

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("unknown", "unknown command fails",
        run_cli("unknown mmap-cli-value.bin") != 0);
    result("missing", "get on a missing value fails",
        run_cli("get mmap-cli-value.bin") != 0);
    result("fixture", "binary input fixture is created", create_input());
    result("set", "set persists binary input",
        run_cli("set mmap-cli-value.bin -i mmap-cli-input.bin") == 0);
    result("get", "get reproduces binary bytes",
        get_matches("mmap-cli-value.bin"));
    result("del", "del removes the stored value",
        run_cli("del mmap-cli-value.bin") == 0 &&
        run_cli("get mmap-cli-value.bin") != 0);

    remove("mmap-cli-value.bin");
    remove("mmap-cli-input.bin");
    remove("mmap-cli-output.bin");

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
