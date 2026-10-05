/**
 * test.c - hnsw CLI contract tests.
 * Summary: Validates one-shot vector search and input precedence.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "hnsw"
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
    const char *stdin_path = "hnsw-cli-stdin.txt";
    const char *output_path = "hnsw-cli-output.txt";
    char command[8192];
    FILE *file;
    size_t size;

    if (stdin_text && !write_text_file(stdin_path, stdin_text)) return 0;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"\"%s\" %s %s> %s 2>NUL\"",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< hnsw-cli-stdin.txt " : "",
        output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< hnsw-cli-stdin.txt " : "",
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
    char command[8192];

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
 * Runs all hnsw CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    const char *file_path = "hnsw-cli-file.csv";
    const char *dataset = "a,1,0\nb,0,1\n";
    char output[4096];
    int fixture_ok;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("search", "literal dataset searches successfully",
        capture_output("a,1,0 -q 1,0 -d 2", NULL,
            output, sizeof(output)) && strncmp(output, "a,", 2) == 0);
    result("stdin", "stdin dataset searches successfully",
        capture_output("-q 1,0 -d 2", dataset,
            output, sizeof(output)) && strncmp(output, "a,", 2) == 0);

    fixture_ok = write_text_file(file_path, dataset);
    result("input file", "-i reads vectors when higher-priority input is absent",
        fixture_ok && capture_output("-i hnsw-cli-file.csv -q 1,0 -d 2", NULL,
            output, sizeof(output)) && strncmp(output, "a,", 2) == 0);
    result("stdin priority", "stdin takes priority over -i",
        fixture_ok && capture_output("-i hnsw-cli-file.csv -q 0,1 -d 2",
            "x,0,1\n", output, sizeof(output)) && strncmp(output, "x,", 2) == 0);
    result("argument priority", "argument takes priority over stdin and -i",
        fixture_ok && capture_output(
            "z,1,0 -i hnsw-cli-file.csv -q 1,0 -d 2",
            "x,0,1\n", output, sizeof(output)) && strncmp(output, "z,", 2) == 0);
    remove(file_path);

    result("l2", "l2 metric searches successfully",
        capture_output("a,0,0 -q 0,0 -d 2 -m l2", NULL,
            output, sizeof(output)) && strncmp(output, "a,", 2) == 0);
    result("limit", "limit option is accepted",
        run_cli("a,1,0 -q 1,0 -d 2 -l 1") == 0);
    result("dimension", "invalid vector dimension fails",
        run_cli("a,1,0 -q 1,0,0 -d 2") != 0);
    result("missing query", "missing query fails",
        run_cli("a,1,0 -d 2") != 0);
    result("unknown", "unknown option fails",
        run_cli("a,1,0 -q 1,0 -d 2 --unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
