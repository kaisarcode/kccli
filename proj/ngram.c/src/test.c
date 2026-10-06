/**
 * test.c - ngram CLI contract tests.
 * Summary: Validates traversal inputs, output, and filter behavior.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "ngram"
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
    const char *stdin_path = "ngram-cli-stdin.txt";
    const char *output_path = "ngram-cli-output.txt";
    char command[4096];
    FILE *file;
    size_t size;

    if (stdin_text && !write_text_file(stdin_path, stdin_text)) return 0;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"\"%s\" %s %s> %s 2>NUL\"",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< ngram-cli-stdin.txt " : "",
        output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< ngram-cli-stdin.txt " : "",
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
 * Removes carriage returns from captured output.
 * @param text Mutable output string.
 * @return None.
 */
static void normalize_newlines(char *text)
{
    size_t read_at = 0;
    size_t write_at = 0;

    while (text[read_at] != '\0') {
        if (text[read_at] != '\r') {
            text[write_at++] = text[read_at];
        }
        read_at++;
    }
    text[write_at] = '\0';
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
    printf("[%d/11] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs all ngram CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    const char *file_path = "ngram-cli-file.txt";
    char output[2048];
    char expected[2048];
    int fixture_ok;

#ifdef _WIN32
    const char *filter_command = "more";
#else
    const char *filter_command = "cat";
#endif

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("unknown", "unknown option fails",
        run_cli("\"a b\" --unknown") != 0);

    strcpy(expected, "a b\nb c\n");
    result("literal", "literal text traverses requested window size",
        capture_output("\"a b c\" --max 2 --min 2", NULL,
            output, sizeof(output)) &&
        (normalize_newlines(output), strcmp(output, expected) == 0));

    result("stdin", "stdin provides text input",
        capture_output("--max 2 --min 2", "a b c",
            output, sizeof(output)) &&
        (normalize_newlines(output), strcmp(output, expected) == 0));

    fixture_ok = write_text_file(file_path, "a b c");
    result("input file", "-i reads text from file",
        fixture_ok && capture_output("-i ngram-cli-file.txt --max 2 --min 2",
            NULL, output, sizeof(output)) &&
        (normalize_newlines(output), strcmp(output, expected) == 0));

    strcpy(expected, "a,b\nb,c\n");
    result("separators", "custom separators delimit tokens",
        capture_output("\"a,b,c\" --max 2 --min 2 --separators \",\"",
            NULL, output, sizeof(output)) &&
        (normalize_newlines(output), strcmp(output, expected) == 0));

    snprintf(expected, sizeof(expected), "\"a b c\" --filter %s",
        filter_command);
    result("filter", "filter output closes the visited span",
        capture_output(expected, NULL, output, sizeof(output)) &&
        (normalize_newlines(output), strcmp(output, "a b c\n") == 0));

    result("bad min", "--min rejects zero",
        run_cli("\"a b\" --min 0") != 0);
    result("range", "nonzero --max cannot be smaller than --min",
        run_cli("\"a b c\" --max 1 --min 2") != 0);
    result("missing filter", "--filter requires a command",
        run_cli("\"a b\" --filter") != 0);

    remove(file_path);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
