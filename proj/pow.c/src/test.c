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
#define CAT_FILE "type"
#else
#define DEV_NULL "/dev/null"
#define CAT_FILE "cat"
#endif

#define POW_TEST_COUNT 29

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
 * Runs one complete shell command.
 * @param command Shell command to run.
 * @return Exit status.
 */
static int run_shell(const char *command)
{
    if (!command || !command[0]) return 1;
    return system(command);
}

/**
 * Writes arbitrary malformed bytes to one file.
 * @return Non-zero on success.
 */
static int write_junk(const char *path)
{
    FILE *file;

    if (!path) return 0;
    file = fopen(path, "wb");
    if (!file) return 0;
    if (fwrite("junk", 1, 4, file) != 4) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

/**
 * Checks whether a file contains one text fragment.
 * @param path File to inspect.
 * @param text Expected fragment.
 * @return Non-zero when the fragment is present.
 */
static int file_contains(const char *path, const char *text)
{
    char buffer[256];
    FILE *file;
    size_t size;

    if (!path || !text) return 0;
    file = fopen(path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, sizeof(buffer) - 1U, file);
    fclose(file);
    buffer[size] = '\0';
    return strstr(buffer, text) != NULL;
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
    result("backend-auto", "creates a challenge with AUTO",
        run("challenge 1 --backend auto > pow-test-challenge-auto.bin") == 0);
    result("backend-sha256", "creates a challenge with explicit SHA-256",
        run("challenge 1 --backend sha256 > pow-test-challenge-sha256.bin") == 0);
    result("backend-short", "creates a challenge with short SHA-256 option",
        run("challenge 1 -b sha256 > pow-test-challenge-short.bin") == 0);
    result("backend-invalid", "unknown backend fails with a diagnostic",
        run("challenge 1 --backend unknown > " DEV_NULL " 2> pow-test-backend.err") != 0 &&
        file_contains("pow-test-backend.err", "invalid backend: unknown"));
    result("backend-missing", "missing long backend value fails",
        run("challenge 1 --backend > " DEV_NULL " 2>&1") != 0);
    result("backend-short-missing", "missing short backend value fails",
        run("challenge 1 -b > " DEV_NULL " 2>&1") != 0);
    result("backend-duplicate", "duplicate backend option fails",
        run("challenge 1 --backend sha256 --backend hashwx > " DEV_NULL " 2>&1") != 0);
    result("solve-backend", "solve rejects backend selection",
        run("solve --backend sha256 > " DEV_NULL " 2> pow-test-solve-backend.err") != 0 &&
        file_contains("pow-test-solve-backend.err", "only valid for challenge"));
    result("verify-backend", "verify rejects backend selection",
        run("verify --backend sha256 -c missing > " DEV_NULL " 2> pow-test-verify-backend.err") != 0 &&
        file_contains("pow-test-verify-backend.err", "only valid for challenge"));
    result("challenge-a", "creates challenge A",
        run("challenge 4 > pow-test-challenge-a.bin") == 0);
    result("challenge-b", "creates hard challenge B",
        run("challenge 18446744073709551615 > pow-test-challenge-b.bin") == 0);
    result("solve-file", "solves a challenge from a file",
        run("solve -i pow-test-challenge-a.bin > pow-test-solution-a.bin") == 0);
    result("verify-file", "accepts a matching solution from a file",
        run("verify -c pow-test-challenge-a.bin -i pow-test-solution-a.bin") == 0);
    result("solve-stdin", "solves a challenge from standard input",
        run_shell(CAT_FILE " pow-test-challenge-a.bin | " KC_CLI_PATH
            " solve > pow-test-solution-stdin.bin") == 0);
    result("verify-stdin", "verifies a solution from standard input",
        run_shell(CAT_FILE " pow-test-solution-stdin.bin | " KC_CLI_PATH
            " verify -c pow-test-challenge-a.bin") == 0);
    result("stdin-precedence", "standard input wins over --input",
        run_shell(CAT_FILE " pow-test-challenge-a.bin | " KC_CLI_PATH
            " solve -i pow-test-challenge-b.bin > "
            "pow-test-solution-precedence.bin") == 0);
    result("stdin-precedence-proof", "precedence solution verifies against stdin challenge",
        run("verify -c pow-test-challenge-a.bin -i pow-test-solution-precedence.bin") == 0);
    result("mismatched-proof", "rejects a valid solution for another challenge",
        run("verify -c pow-test-challenge-b.bin -i pow-test-solution-a.bin > "
            DEV_NULL " 2> pow-test-invalid.err") != 0 &&
        file_contains("pow-test-invalid.err", "invalid solution"));
    result("malformed-data", "creates unrelated malformed input",
        write_junk("pow-test-junk.bin"));
    result("solve-malformed", "solve rejects malformed challenge",
        run("solve -i pow-test-junk.bin > " DEV_NULL " 2>&1") != 0);
    result("verify-malformed", "verify rejects malformed solution",
        run("verify -c pow-test-challenge-a.bin -i pow-test-junk.bin > "
            DEV_NULL " 2>&1") != 0);
    result("verify-options", "verify requires a challenge",
        run("verify -i pow-test-solution-a.bin > " DEV_NULL " 2>&1") != 0);

    remove("pow-test-invalid.err");
    remove("pow-test-backend.err");
    remove("pow-test-solve-backend.err");
    remove("pow-test-verify-backend.err");
    remove("pow-test-challenge-short.bin");
    remove("pow-test-challenge-sha256.bin");
    remove("pow-test-challenge-auto.bin");
    remove("pow-test-junk.bin");
    remove("pow-test-solution-precedence.bin");
    remove("pow-test-solution-stdin.bin");
    remove("pow-test-solution-a.bin");
    remove("pow-test-challenge-b.bin");
    remove("pow-test-challenge-a.bin");

    printf("\n%d passed, %d failed\n",
        POW_TEST_COUNT - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
