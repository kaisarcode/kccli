/**
 * test.c - REDP2P CLI contract tests.
 * Summary: Checks the public command surface of the REDP2P CLI.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
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
static void test_result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf("[%d/%d] [%s] %s: %s\n",
        test_current, test_total, ok ? "PASS" : "FAIL", name, detail);
}

/**
 * Executes the CLI while discarding child output.
 * @param args Arguments appended after the executable.
 * @return Child exit code, or -1 on execution failure.
 */
static int test_run(const char *args)
{
#ifdef _WIN32
    HANDLE null_handle;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    char command[4096];
    DWORD exit_code;
    int n;

    n = snprintf(command, sizeof(command), "\"%s\" %s",
        KC_CLI_PATH, args ? args : "");
    if (n < 0 || (size_t)n >= sizeof(command)) return -1;

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

    if (!CreateProcessA(NULL, command, NULL, NULL, TRUE, 0,
        NULL, NULL, &startup, &process))
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
    int n;
    int status;

    n = snprintf(command, sizeof(command),
        "\"%s\" %s > /dev/null 2>&1",
        KC_CLI_PATH, args ? args : "");
    if (n < 0 || (size_t)n >= sizeof(command)) return -1;
    status = system(command);
    if (status == -1 || !WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
#endif
}

/**
 * Runs the REDP2P CLI contract.
 * @return Process exit code.
 */
int main(void)
{
    test_total = 5;
    test_current = 0;
    test_failures = 0;

    test_result("help", "prints command help", test_run("--help") == 0);
    test_result("version", "reports the linked REDP2P build",
        test_run("--version") == 0);
    test_result("unknown", "rejects an unknown command",
        test_run("unknown") != 0);
    test_result("idx", "rejects an invalid index port",
        test_run("idx 0") != 0);
    test_result("con", "does not accept consumer protocol plumbing",
        test_run("con x@127.0.0.1:9876 --tcp 9000") != 0);

    printf("\n%d passed, %d failed\n",
        test_total - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
