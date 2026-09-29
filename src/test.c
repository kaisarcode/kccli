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
 * Remove the isolated metadata directory.
 * @param dir Directory path.
 * @return None.
 */
static void fixture_remove(const char *dir) {
    if (!dir || !dir[0]) return;
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
    char dir[1024] = {0};
    int fixture_ok;

    test_total = 4;
    test_current = 0;
    test_failures = 0;

    result("help", "prints command help", run_cli("--help") == 0);
    result("version", "reports linked libinit build version",
        run_cli("--version") == 0);
    result("errors", "rejects an unknown option", run_cli("--unknown") != 0);

    fixture_ok = fixture_create(dir, sizeof(dir)) == 0;
    result("list", "lists an isolated empty namespace",
        fixture_ok && run_cli("--list") == 0);
    fixture_remove(dir);

    printf("\n%d passed, %d failed\n",
        test_total - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
