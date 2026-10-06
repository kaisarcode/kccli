/**
 * test.c - kccli init contract tests.
 * Summary: Checks the statically linked init CLI surface.
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
#else
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "init"
#endif

static int test_total;
static int test_current;
static int test_failures;

/**
 * Print one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Whether the test passed.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok) {
    test_current++;
    if (!ok) test_failures++;
    printf(
        "[%d/%d] [%s] %s: %s\n",
        test_current,
        test_total,
        ok ? "PASS" : "FAIL",
        name,
        detail
    );
}

/**
 * Execute the CLI with output discarded.
 * @param args Arguments appended after the executable.
 * @return Child process exit code, or -1 on execution failure.
 */
static int run_cli(const char *args) {
#ifdef _WIN32
    HANDLE null_handle;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char command_line[4096];
    DWORD exit_code;

    if ((size_t)snprintf(
            command_line,
            sizeof(command_line),
            "\"%s\" %s",
            KC_CLI_PATH,
            args ? args : ""
        ) >= sizeof(command_line)) {
        return -1;
    }

    null_handle = CreateFileA(
        "NUL",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (null_handle == INVALID_HANDLE_VALUE) return -1;

    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = null_handle;
    startup.hStdError = null_handle;

    if (!CreateProcessA(
            NULL,
            command_line,
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &startup,
            &process)) {
        CloseHandle(null_handle);
        return -1;
    }

    WaitForSingleObject(process.hProcess, INFINITE);
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        exit_code = (DWORD)-1;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(null_handle);
    return (int)exit_code;
#else
    char command[8192];
    int rc;

    if ((size_t)snprintf(
            command,
            sizeof(command),
            "\"%s\" %s > /dev/null 2>&1",
            KC_CLI_PATH,
            args ? args : ""
        ) >= sizeof(command)) {
        return -1;
    }

    rc = system(command);
    if (rc == -1 || !WIFEXITED(rc)) return -1;
    return WEXITSTATUS(rc);
#endif
}

/**
 * Execute the CLI and capture standard output.
 * @param args Arguments appended after the executable.
 * @param buffer Output buffer.
 * @param buffer_size Output buffer capacity.
 * @return Nonzero on success.
 */
static int capture_output(
    const char *args,
    char *buffer,
    size_t buffer_size
) {
    const char *output_path = "init-cli-output.txt";
    char command[8192];
    FILE *file;
    size_t size;

#ifdef _WIN32
    if ((size_t)snprintf(
            command,
            sizeof(command),
            "\"\"%s\" %s > %s 2>NUL\"",
            KC_CLI_PATH,
            args ? args : "",
            output_path
        ) >= sizeof(command)) {
        return 0;
    }
#else
    if ((size_t)snprintf(
            command,
            sizeof(command),
            "\"%s\" %s > %s 2>/dev/null",
            KC_CLI_PATH,
            args ? args : "",
            output_path
        ) >= sizeof(command)) {
        return 0;
    }
#endif
    if (system(command) != 0) {
        remove(output_path);
        return 0;
    }
    file = fopen(output_path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, buffer_size - 1U, file);
    fclose(file);
    remove(output_path);
    buffer[size] = '\0';
    return 1;
}

/**
 * Check one captured single-line output value.
 * @param output Captured output.
 * @param expected Expected line without newline.
 * @return Nonzero when the line matches exactly.
 */
static int output_matches(const char *output, const char *expected) {
    size_t size;

    if (!output || !expected) return 0;
    size = strlen(output);
    while (size > 0U &&
            (output[size - 1U] == '\n' || output[size - 1U] == '\r')) {
        size--;
    }
    return strlen(expected) == size && strncmp(output, expected, size) == 0;
}

/**
 * Join a fixture path.
 * @param out Output path buffer.
 * @param cap Output capacity.
 * @param dir Fixture directory.
 * @param name File name.
 * @return Zero on success, nonzero on overflow.
 */
static int fixture_path(
    char *out,
    size_t cap,
    const char *dir,
    const char *name
) {
#ifdef _WIN32
    return (size_t)snprintf(out, cap, "%s\\%s", dir, name) < cap ? 0 : 1;
#else
    return (size_t)snprintf(out, cap, "%s/%s", dir, name) < cap ? 0 : 1;
#endif
}

/**
 * Create an isolated metadata directory and export KC_INIT_DIR.
 * @param out Destination path.
 * @param cap Destination capacity.
 * @return 0 on success, 1 on failure.
 */
static int fixture_create(char *out, size_t cap) {
#ifdef _WIN32
    char base[MAX_PATH];
    char temp[MAX_PATH];

    if (!GetTempPathA((DWORD)sizeof(base), base)) return 1;
    if (!GetTempFileNameA(base, "kcc", 0, temp)) return 1;
    (void)DeleteFileA(temp);
    if (!CreateDirectoryA(temp, NULL)) return 1;
    if (strlen(temp) + 1U > cap) {
        (void)RemoveDirectoryA(temp);
        return 1;
    }
    strcpy(out, temp);
    return _putenv_s("KC_INIT_DIR", out) == 0 ? 0 : 1;
#else
    char temp[] = "/tmp/kccli-init-XXXXXX";
    char *dir = mkdtemp(temp);

    if (!dir || strlen(dir) + 1U > cap) return 1;
    strcpy(out, dir);
    return setenv("KC_INIT_DIR", out, 1) == 0 ? 0 : 1;
#endif
}

/**
 * Write one fixture file.
 * @param dir Fixture directory.
 * @param name File name.
 * @param value File contents.
 * @return Zero on success, nonzero on failure.
 */
static int fixture_write(
    const char *dir,
    const char *name,
    const char *value
) {
    char path[1024];
    FILE *file;

    if (fixture_path(path, sizeof(path), dir, name) != 0) return 1;
    file = fopen(path, "wb");
    if (!file) return 1;
    fputs(value, file);
    fclose(file);
    return 0;
}

/**
 * Remove the isolated metadata directory.
 * @param dir Directory path.
 * @return None.
 */
static void fixture_remove(const char *dir) {
    static const char *files[] = {
        "sample",
        "sample.user",
        "sample.backend"
    };
    char path[1024];
    size_t i;

    if (!dir || !dir[0]) return;
    for (i = 0U; i < sizeof(files) / sizeof(files[0]); i++) {
        if (fixture_path(path, sizeof(path), dir, files[i]) == 0) {
            (void)remove(path);
        }
    }
#ifdef _WIN32
    (void)RemoveDirectoryA(dir);
#else
    (void)rmdir(dir);
#endif
}

/**
 * Run all CLI contract cases.
 * @return Process status.
 */
int main(void) {
    const char *expected = "sample\ttester\techo init-test";
    char output[4096];
    char dir[1024] = {0};
    int fixture_ok;

    test_total = 5;
    test_current = 0;
    test_failures = 0;

    result("help", "prints command help", run_cli("--help") == 0);
    result("version", "reports linked libinit build version",
        run_cli("--version") == 0);
    result("errors", "rejects an unknown option", run_cli("--unknown") != 0);

    fixture_ok = fixture_create(dir, sizeof(dir)) == 0 &&
        fixture_write(dir, "sample", "echo init-test") == 0 &&
        fixture_write(dir, "sample.user", "tester") == 0 &&
        fixture_write(dir, "sample.backend", "1") == 0;

    result(
        "list",
        "list emits fixed id, user, and command columns",
        fixture_ok && capture_output("-l", output, sizeof(output)) &&
        output_matches(output, expected)
    );
    result(
        "list one",
        "list-one uses the same fixed column schema",
        fixture_ok && capture_output("-l sample", output, sizeof(output)) &&
        output_matches(output, expected)
    );
    fixture_remove(dir);

    printf("\n%d passed, %d failed\n",
        test_total - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
