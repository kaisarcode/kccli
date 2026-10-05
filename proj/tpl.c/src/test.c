/**
 * test.c - tpl CLI contract tests.
 * Summary: Validates template rendering, options, and input precedence.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "tpl"
#endif

static int test_total = 0;
static int test_failed = 0;

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

static int output_equals(
    const char *args,
    const char *stdin_text,
    const char *expected
)
{
    const char *input_path = "tpl-cli-stdin.txt";
    const char *output_path = "tpl-cli-output.txt";
    char command[4096];
    char buffer[4096];
    FILE *file;
    size_t size;
    int status;

    if (stdin_text && !write_text_file(input_path, stdin_text)) return 0;

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>NUL",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< tpl-cli-stdin.txt " : "",
        output_path);
#else
    snprintf(command, sizeof(command),
        "\"%s\" %s %s> %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        stdin_text ? "< tpl-cli-stdin.txt " : "",
        output_path);
#endif

    status = system(command);
    if (stdin_text) remove(input_path);
    if (status != 0) {
        remove(output_path);
        return 0;
    }

    file = fopen(output_path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    remove(output_path);
    buffer[size] = '\0';
    return strcmp(buffer, expected) == 0;
}

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

static void result(const char *name, const char *detail, int ok)
{
    test_total++;
    if (!ok) test_failed++;
    printf("[%d/11] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

int main(void)
{
    const char *file_path = "tpl-cli-input.tpl";
    int fixture_ok;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing", "missing template input fails", run_cli("") != 0);
    result("root", "--root requires a value", run_cli("Hello --root") != 0);
    result("var", "--var requires key and value", run_cli("Hello --var key") != 0);
    result("unknown", "unknown option fails", run_cli("Hello --unknown") != 0);
    result("argument", "positional argument is literal template text",
        output_equals("Hello", NULL, "Hello"));
    result("stdin", "stdin provides template input",
        output_equals("", "Hello", "Hello"));

    fixture_ok = write_text_file(file_path, "File");
    result("input file", "-i reads template file when higher-priority input is absent",
        fixture_ok && output_equals("-i tpl-cli-input.tpl", NULL, "File"));
    result("stdin priority", "stdin takes priority over -i",
        fixture_ok && output_equals("-i tpl-cli-input.tpl", "Stdin",
            "Stdin"));
    result("argument priority", "argument takes priority over stdin and -i",
        fixture_ok && output_equals("-i tpl-cli-input.tpl Argument", "Stdin",
            "Argument"));
    remove(file_path);

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
