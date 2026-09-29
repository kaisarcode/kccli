/**
 * test.c - kccli redp2p contract tests.
 * Summary: Checks the statically linked redp2p CLI surface.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "redp2p"
#endif

static int test_total;
static int test_current;
static int test_failures;

/**
 * Prints one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Whether the test passed.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf("[%d/%d] [%s] %s: %s\n", test_current, test_total,
        ok ? "PASS" : "FAIL", name, detail);
}

/**
 * Executes the CLI with output discarded.
 * @param args Arguments appended after the executable.
 * @return Child exit code, or -1 on execution failure.
 */
static int run_cli(const char *args)
{
#ifdef _WIN32
    HANDLE null_handle;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char command_line[4096];
    DWORD exit_code;

    if ((size_t)snprintf(command_line, sizeof(command_line), "\"%s\" %s",
        KC_CLI_PATH, args ? args : "") >= sizeof(command_line))
        return -1;
    null_handle = CreateFileA("NUL", GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (null_handle == INVALID_HANDLE_VALUE) return -1;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = null_handle;
    startup.hStdError = null_handle;
    if (!CreateProcessA(NULL, command_line, NULL, NULL, TRUE, 0, NULL, NULL,
        &startup, &process))
    {
        CloseHandle(null_handle);
        return -1;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    if (!GetExitCodeProcess(process.hProcess, &exit_code))
        exit_code = (DWORD)-1;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(null_handle);
    return (int)exit_code;
#else
    char command[8192];
    int rc;

    if ((size_t)snprintf(command, sizeof(command),
        "\"%s\" %s > /dev/null 2>&1", KC_CLI_PATH,
        args ? args : "") >= sizeof(command))
        return -1;
    rc = system(command);
    if (rc == -1 || !WIFEXITED(rc)) return -1;
    return WEXITSTATUS(rc);
#endif
}

/**
 * Runs all CLI contract cases.
 * @return Process status.
 */
int main(void)
{
    test_total = 6;
    test_current = 0;
    test_failures = 0;

    result("help", "prints command help", run_cli("--help") == 0);
    result("version", "reports linked libredp2p build version",
        run_cli("--version") == 0);
    result("errors", "rejects an unknown command",
        run_cli("unknown") != 0);
    result("idx", "rejects an invalid index port",
        run_cli("idx 0") != 0);
    result("pub", "requires a publisher transport and local port",
        run_cli("pub test@127.0.0.1:1") != 0);
    result("con", "rejects the old consumer protocol syntax",
        run_cli("con test@127.0.0.1:1 --tcp 9000") != 0);

    printf("\n%d passed, %d failed\n",
        test_total - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
