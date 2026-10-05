/**
 * test.c - wch CLI contract tests.
 * Summary: Validates watcher administration and command-line behavior.
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
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#endif

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "wch"
#endif

#define WCH_TEST_COUNT 9

static int test_total = 0;
static int test_failed = 0;

/**
 * Executes the CLI with output discarded.
 * @param args Arguments appended after the executable.
 * @return Child process status from system().
 */
static int run_cli(const char *args)
{
    char command[4096];

#ifdef _WIN32
    snprintf(
        command,
        sizeof(command),
        "\"\"%s\" %s > NUL 2>&1\"",
        KC_CLI_PATH,
        args ? args : ""
    );
#else
    snprintf(
        command,
        sizeof(command),
        "\"%s\" %s > /dev/null 2>&1",
        KC_CLI_PATH,
        args ? args : ""
    );
#endif
    return system(command);
}

/**
 * Executes the CLI and captures standard output.
 * @param args Arguments appended after the executable.
 * @param buffer Output buffer.
 * @param buffer_size Output buffer capacity.
 * @return Nonzero on success.
 */
static int capture_output(
    const char *args,
    char *buffer,
    size_t buffer_size
)
{
    const char *output_path = "wch-cli-output.txt";
    char command[4096];
    FILE *file;
    size_t size;

#ifdef _WIN32
    snprintf(
        command,
        sizeof(command),
        "\"\"%s\" %s > %s 2>NUL\"",
        KC_CLI_PATH,
        args ? args : "",
        output_path
    );
#else
    snprintf(
        command,
        sizeof(command),
        "\"%s\" %s > %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        output_path
    );
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
    printf(
        "[%d/%d] [%s] %s: %s\n",
        test_total,
        WCH_TEST_COUNT,
        ok ? "PASS" : "FAIL",
        name,
        detail
    );
}

/**
 * Sleeps briefly while the resident watcher starts.
 * @return None.
 */
static void test_delay(void)
{
#ifdef _WIN32
    Sleep(300);
#else
    struct timespec delay;

    delay.tv_sec = 0;
    delay.tv_nsec = 300000000L;
    nanosleep(&delay, NULL);
#endif
}

#ifdef _WIN32
/**
 * Starts one streaming watcher process for contract testing.
 * @return Process handle, or NULL on failure.
 */
static HANDLE start_watcher(void)
{
    char command[4096];
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;

    snprintf(command, sizeof(command), "\"%s\" sample .", KC_CLI_PATH);
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    if (!CreateProcessA(
            NULL,
            command,
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW,
            NULL,
            NULL,
            &startup,
            &process
        )) {
        return NULL;
    }
    CloseHandle(process.hThread);
    return process.hProcess;
}

/**
 * Stops one streaming watcher process.
 * @param process Process handle.
 * @return None.
 */
static void stop_watcher(HANDLE process)
{
    if (process == NULL) return;
    TerminateProcess(process, 0);
    WaitForSingleObject(process, INFINITE);
    CloseHandle(process);
}

/**
 * Removes the isolated watcher runtime directory.
 * @return None.
 */
static void clear_runtime(void)
{
    (void)system("rmdir /S /Q wch-cli-runtime >NUL 2>&1");
}
#else
/**
 * Starts one streaming watcher process for contract testing.
 * @return Child process identifier, or negative one on failure.
 */
static pid_t start_watcher(void)
{
    pid_t pid = fork();

    if (pid != 0) return pid;
    (void)freopen("/dev/null", "w", stdout);
    (void)freopen("/dev/null", "w", stderr);
    execl(KC_CLI_PATH, KC_CLI_PATH, "sample", ".", (char *)NULL);
    _exit(127);
}

/**
 * Stops one streaming watcher process.
 * @param pid Child process identifier.
 * @return None.
 */
static void stop_watcher(pid_t pid)
{
    int status;

    if (pid <= 0) return;
    (void)kill(pid, SIGTERM);
    (void)waitpid(pid, &status, 0);
}

/**
 * Removes the isolated watcher runtime directory.
 * @return None.
 */
static void clear_runtime(void)
{
    (void)system("rm -rf wch-cli-runtime");
}
#endif

/**
 * Runs all wch CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    char output[4096];

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing", "missing watcher name fails", run_cli("") != 0);
    result("cmd", "-c is not part of the CLI", run_cli("sample . -c x") != 0);
    result(
        "unknown",
        "unknown option fails",
        run_cli("sample --unknown") != 0
    );
    result(
        "bad name",
        "invalid watcher name fails",
        run_cli("bad/name -d") != 0
    );

#ifdef _WIN32
    if (_putenv_s("KC_WCH_DIR", "wch-cli-runtime") != 0) {
        result("create", "path creates a resident watcher", 0);
        result("list", "list hides internal command and emits TSV", 0);
        result("delete", "watcher deletion succeeds", 0);
    } else {
        HANDLE process;

        clear_runtime();
        process = start_watcher();
        test_delay();
        result("create", "path creates a resident watcher", process != NULL);
        result(
            "list",
            "list hides the internal command and emits TSV",
            capture_output("-l sample", output, sizeof(output)) &&
            strncmp(output, "sample\t.\t0\t1", 12) == 0
        );
        stop_watcher(process);
        result(
            "delete",
            "watcher deletion succeeds",
            run_cli("sample -d") == 0
        );
        clear_runtime();
    }
#else
    if (setenv("KC_WCH_DIR", "wch-cli-runtime", 1) != 0) {
        result("create", "path creates a resident watcher", 0);
        result("list", "list hides internal command and emits TSV", 0);
        result("delete", "watcher deletion succeeds", 0);
    } else {
        pid_t pid;

        clear_runtime();
        pid = start_watcher();
        test_delay();
        result("create", "path creates a resident watcher", pid > 0);
        result(
            "list",
            "list hides the internal command and emits TSV",
            capture_output("-l sample", output, sizeof(output)) &&
            strncmp(output, "sample\t.\t0\t1", 12) == 0
        );
        stop_watcher(pid);
        result(
            "delete",
            "watcher deletion succeeds",
            run_cli("sample -d") == 0
        );
        clear_runtime();
    }
#endif

    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
