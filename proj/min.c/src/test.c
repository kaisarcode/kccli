/**
 * test.c - min CLI contract tests.
 * Summary: Validates minification syntax and execution.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "min"
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
 * Verifies one minification result exactly.
 * @param type Minifier type.
 * @param input Input source.
 * @param expected Expected output.
 * @return Nonzero on success.
 */
static int output_equals(
    const char *type,
    const char *input,
    const char *expected
)
{
    const char *input_path = "min-cli-input.txt";
    const char *output_path = "min-cli-output.txt";
    char command[4096];
    char buffer[4096];
    FILE *file;
    size_t size;
    int ok;

    file = fopen(input_path, "wb");
    if (!file) return 0;
    if (fwrite(input, 1, strlen(input), file) != strlen(input)) {
        fclose(file);
        remove(input_path);
        return 0;
    }
    fclose(file);

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"%s\" -t %s %s > %s 2>NUL",
        KC_CLI_PATH, type, input_path, output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" -t %s %s > %s 2>/dev/null",
        KC_CLI_PATH, type, input_path, output_path);
#endif
    if (system(command) != 0) {
        remove(input_path);
        remove(output_path);
        return 0;
    }

    file = fopen(output_path, "rb");
    if (!file) {
        remove(input_path);
        return 0;
    }
    size = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    remove(input_path);
    remove(output_path);
    buffer[size] = '\0';
    ok = strcmp(buffer, expected) == 0;
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
 * Runs all min CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("css", "CSS minification succeeds",
        output_equals("css", "body { color: red; }", "body{color:red}"));
    result("js", "JavaScript minification succeeds",
        output_equals("js", "const x = 1; // comment\n", "const x=1;"));
    result("html", "HTML minification succeeds",
        output_equals("html", "<div>  hello  </div>", "<div> hello </div>"));
    result("txt", "text minification succeeds",
        output_equals("txt", "  hello   world  ", "hello world"));
    result("missing type", "type is required", run_cli("") != 0);
    result("bad type", "unknown type fails", run_cli("-t xml") != 0);
    result("unknown", "unknown option fails", run_cli("-t txt --unknown") != 0);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
