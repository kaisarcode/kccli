/**
 * test.c - emb CLI contract tests.
 * Summary: Validates embedding input precedence and output.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "emb"
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
    const char *stdin_path = "emb-cli-stdin.txt";
    const char *output_path = "emb-cli-output.txt";
    char command[4096];
    FILE *file;
    size_t size;

    if (stdin_text && !write_text_file(stdin_path, stdin_text)) return 0;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"\"%s\" %s %s> %s 2>NUL\"",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< emb-cli-stdin.txt " : "",
        output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< emb-cli-stdin.txt " : "",
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
 * Verifies that embedding output is headerless CSV.
 * @return Nonzero on success.
 */
static int csv_output_ok(void)
{
    char buffer[8192];
    int ok;

    if (!capture_output("\"Hello world\"", NULL,
        buffer, sizeof(buffer))) return 0;
    ok = buffer[0] != '\0' && strchr(buffer, ',') != NULL &&
        strchr(buffer, '\t') == NULL;
    if (strncmp(buffer, "code,", 5) == 0 || strncmp(buffer, "value,", 6) == 0)
        ok = 0;
    return ok;
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
    printf("[%d/9] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Runs all emb CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    const char *file_path = "emb-cli-file.txt";
    char expected[8192];
    char actual[8192];
    int fixture_ok;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("dimension", "--dimension succeeds", run_cli("--dimension") == 0);
    result("embed", "text embedding succeeds", run_cli("\"Hello world\"") == 0);
    result("csv", "embedding output is headerless CSV", csv_output_ok());

    fixture_ok = write_text_file(file_path, "File input");
    result("input file", "-i embeds file input",
        fixture_ok && run_cli("-i emb-cli-file.txt") == 0);

    result("stdin priority", "stdin takes priority over -i",
        fixture_ok && capture_output(NULL, "Stdin input",
            expected, sizeof(expected)) &&
        capture_output("-i emb-cli-file.txt", "Stdin input",
            actual, sizeof(actual)) && strcmp(expected, actual) == 0);

    result("argument priority", "argument takes priority over stdin and -i",
        fixture_ok && capture_output("\"Argument input\"", NULL,
            expected, sizeof(expected)) &&
        capture_output("\"Argument input\" -i emb-cli-file.txt", "Stdin input",
            actual, sizeof(actual)) && strcmp(expected, actual) == 0);
    remove(file_path);

    result("unknown", "unknown option fails",
        run_cli("\"Hello world\" --unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
