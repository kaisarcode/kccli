/**
 * test.c - tpm CLI contract tests.
 * Summary: Validates text profile matching inputs and execution.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "tpm"
#endif

static int test_total = 0;
static int test_failed = 0;

/**
 * Writes one text fixture file.
 * @param path Output file path.
 * @param text Text to write.
 * @return Nonzero on success.
 */
static int write_text_file(const char *path, const char *text)
{
    FILE *file = fopen(path, "wb");
    size_t size = strlen(text);
    int ok;

    if (!file) return 0;
    ok = fwrite(text, 1, size, file) == size;
    fclose(file);
    return ok;
}

/**
 * Executes the CLI and captures stdout.
 * @param args CLI arguments.
 * @param stdin_text Optional stdin content.
 * @param buffer Output buffer.
 * @param buffer_size Output buffer size.
 * @return Nonzero on success.
 */
static int capture_output(
    const char *args,
    const char *stdin_text,
    char *buffer,
    size_t buffer_size
)
{
    const char *stdin_path = "tpm-cli-stdin.txt";
    const char *output_path = "tpm-cli-output.txt";
    char command[4096];
    FILE *file;
    size_t size;

    if (stdin_text && !write_text_file(stdin_path, stdin_text)) return 0;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"\"%s\" %s %s> %s 2>NUL\"",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< tpm-cli-stdin.txt " : "",
        output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< tpm-cli-stdin.txt " : "",
        output_path);
#endif

    if (system(command) != 0) {
        if (stdin_text) remove(stdin_path);
        remove(output_path);
        return 0;
    }
    if (stdin_text) remove(stdin_path);

    file = fopen(output_path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, buffer_size - 1, file);
    fclose(file);
    remove(output_path);
    buffer[size] = '\0';
    return 1;
}

/**
 * Executes the CLI with output discarded.
 * @param args Arguments appended after executable.
 * @return Child process status from system().
 */
static int run_cli(const char *args)
{
    char command[4096];

#ifdef _WIN32
    snprintf(command, sizeof(command), "\"\"%s\" %s > NUL 2>&1\"",
        KC_CLI_PATH, args ? args : "");
#else
    snprintf(command, sizeof(command), "\"%s\" %s > /dev/null 2>&1",
        KC_CLI_PATH, args ? args : "");
#endif
    return system(command);
}

/**
 * Verifies that score output is one number in the public range.
 * @return Nonzero on success.
 */
static int score_output_ok(void)
{
    char buffer[256];
    char *end = NULL;
    double score;

    if (!capture_output("\"hello world\" \"hello world\"", NULL,
        buffer, sizeof(buffer))) return 0;
    score = strtod(buffer, &end);
    if (end == buffer || score < 0.0 || score > 1.0) return 0;
    while (*end == '\r' || *end == '\n') end++;
    return *end == '\0';
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
    printf("[%d/12] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs all tpm CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    const char *file_path = "tpm-cli-file.txt";
    char expected[256];
    char actual[256];
    int fixture_ok;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing map", "map text is required", run_cli("") != 0);
    result("score", "map and input text score successfully",
        run_cli("\"hello world\" \"hello world\"") == 0);
    result("output", "score is one number in range 0..1", score_output_ok());

    fixture_ok = write_text_file(file_path, "completely different text");
    result("input file", "-i reads input text from file",
        fixture_ok && run_cli("\"hello world\" -i tpm-cli-file.txt") == 0);
    result("stdin priority", "stdin takes priority over -i",
        fixture_ok && capture_output("\"hello world\"", "hello world",
            expected, sizeof(expected)) &&
        capture_output("\"hello world\" -i tpm-cli-file.txt", "hello world",
            actual, sizeof(actual)) && strcmp(expected, actual) == 0);
    result("argument priority", "argument takes priority over stdin and -i",
        fixture_ok && capture_output(
            "\"hello world\" \"hello world\"", NULL,
            expected, sizeof(expected)) &&
        capture_output(
            "\"hello world\" \"hello world\" -i tpm-cli-file.txt",
            "completely different text", actual, sizeof(actual)) &&
        strcmp(expected, actual) == 0);
    remove(file_path);

    result("ngram", "positive ngram size is accepted by the CLI",
        run_cli("\"hello world\" \"hello world\" --ngram 4") == 0);
    result("bad ngram", "non-positive ngram size fails",
        run_cli("\"hello world\" \"hello world\" --ngram 0") != 0);
    result("missing value", "--input requires a following file",
        run_cli("\"hello world\" --input") != 0);
    result("unknown", "unknown option fails",
        run_cli("\"hello world\" \"hello world\" --unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
