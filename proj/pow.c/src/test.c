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

#define POW_TEST_COUNT 13

static int test_current;
static int test_failures;

/**
 * Prints one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Non-zero when the test passed.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf("[%d/%d] [%s] %s: %s\n", test_current, POW_TEST_COUNT,
        ok ? "PASS" : "FAIL", name, detail);
}

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
 * Writes an invalid solution derived from a valid solution file.
 * @return Non-zero on success.
 */
static int write_invalid_solution(void)
{
    const char *source = "pow-test-solution.bin";
    const char *destination = "pow-test-invalid.bin";
    unsigned char buffer[16];
    FILE *file;

    file = fopen(source, "rb");
    if (!file) return 0;
    if (fread(buffer, 1, sizeof(buffer), file) != sizeof(buffer)) {
        fclose(file);
        return 0;
    }
    fclose(file);
    buffer[4] = 3;
    file = fopen(destination, "wb");
    if (!file) return 0;
    if (fwrite(buffer, 1, sizeof(buffer), file) != sizeof(buffer)) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

/**
 * Test entry point.
 * @return Exit status.
 */
int main(void)
{
    test_current = 0;
    test_failures = 0;

    result("help", "--help succeeds", run("--help > " DEV_NULL) == 0);
    result("version", "--version succeeds", run("--version > " DEV_NULL) == 0);
    result("missing", "missing command fails",
        run("> " DEV_NULL " 2>&1") != 0);
    result("unknown", "unknown command fails", run("unknown > " DEV_NULL " 2>&1") != 0);
    result("effort-zero", "zero effort fails",
        run("challenge 0 > " DEV_NULL " 2>&1") != 0);
    result("effort-text", "non-numeric effort fails",
        run("challenge nope > " DEV_NULL " 2>&1") != 0);
    result("challenge-options", "challenge rejects input option",
        run("challenge 1 -i input > " DEV_NULL " 2>&1") != 0);
    result("challenge", "creates a binary challenge",
        run("challenge 4 > pow-test-challenge.bin") == 0);
    result("solve", "solves a challenge from a file",
        run("solve -i pow-test-challenge.bin > pow-test-solution.bin") == 0);
    result("verify", "accepts the matching solution",
        run("verify -c pow-test-challenge.bin -i pow-test-solution.bin") == 0);
    result("invalid-solution", "rejects malformed solution",
        write_invalid_solution() && run(
            "verify -c pow-test-challenge.bin -i pow-test-invalid.bin > "
            DEV_NULL " 2>&1") != 0);
    result("verify-options", "verify requires a challenge",
        run("verify -i pow-test-solution.bin > " DEV_NULL " 2>&1") != 0);
    result("solve-input", "solve rejects malformed input",
        run("solve -i pow-test-invalid.bin > " DEV_NULL " 2>&1") != 0);

    remove("pow-test-invalid.bin");
    remove("pow-test-solution.bin");
    remove("pow-test-challenge.bin");

    printf("\n%d passed, %d failed\n",
        POW_TEST_COUNT - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
