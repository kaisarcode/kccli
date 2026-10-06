/**
 * test.c - netl CLI contract tests.
 * Summary: Validates listener dispatch and command-line behavior.
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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET netl_test_fd_t;
#define NETL_TEST_INVALID INVALID_SOCKET
#define NETL_TEST_CLOSE(fd) closesocket(fd)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
typedef int netl_test_fd_t;
#define NETL_TEST_INVALID (-1)
#define NETL_TEST_CLOSE(fd) close(fd)
#endif

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "netl"
#endif

#define NETL_TEST_COUNT 9

static int test_current;
static int test_failures;

/**
 * Print one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Whether the test passed.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf(
        "[%d/%d] [%s] %s: %s\n",
        test_current,
        NETL_TEST_COUNT,
        ok ? "PASS" : "FAIL",
        name,
        detail
    );
}

/**
 * Execute the CLI with output discarded.
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
 * Sleep for a short test interval.
 * @param milliseconds Delay in milliseconds.
 * @return None.
 */
static void test_sleep(unsigned long milliseconds)
{
#ifdef _WIN32
    Sleep((DWORD)milliseconds);
#else
    struct timespec delay;

    delay.tv_sec = (time_t)(milliseconds / 1000UL);
    delay.tv_nsec = (long)((milliseconds % 1000UL) * 1000000UL);
    (void)nanosleep(&delay, NULL);
#endif
}

/**
 * Reserve and release one local TCP port for the listener test.
 * @return Available port, or zero on failure.
 */
static unsigned short test_port(void)
{
    struct sockaddr_in address;
    netl_test_fd_t fd;
    socklen_t size;
    unsigned short port;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == NETL_TEST_INVALID) return 0U;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    if (bind(
            fd,
            (const struct sockaddr *)&address,
            (socklen_t)sizeof(address)
        ) != 0) {
        NETL_TEST_CLOSE(fd);
        return 0U;
    }
    size = (socklen_t)sizeof(address);
    if (getsockname(fd, (struct sockaddr *)&address, &size) != 0) {
        NETL_TEST_CLOSE(fd);
        return 0U;
    }
    port = ntohs(address.sin_port);
    NETL_TEST_CLOSE(fd);
    return port;
}

#ifdef _WIN32
typedef PROCESS_INFORMATION netl_test_process_t;
#else
typedef pid_t netl_test_process_t;
#endif

/**
 * Start a TCP netl process for the integration case.
 * @param port Listener port.
 * @param process Output process handle.
 * @return Nonzero on success.
 */
static int test_start_listener(
    unsigned short port,
    netl_test_process_t *process
)
{
    char port_text[16];

    snprintf(port_text, sizeof(port_text), "%u", (unsigned int)port);
#ifdef _WIN32
    {
        STARTUPINFOA startup;
        char command[4096];

        memset(&startup, 0, sizeof(startup));
        memset(process, 0, sizeof(*process));
        startup.cb = sizeof(startup);
        snprintf(
            command,
            sizeof(command),
            "\"%s\" %s cmd /C more -t tcp",
            KC_CLI_PATH,
            port_text
        );
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
                process
            )) {
            return 0;
        }
        CloseHandle(process->hThread);
        process->hThread = NULL;
        return 1;
    }
#else
    *process = fork();
    if (*process < 0) return 0;
    if (*process == 0) {
        execl(
            KC_CLI_PATH,
            KC_CLI_PATH,
            port_text,
            "cat",
            "-t",
            "tcp",
            (char *)NULL
        );
        _exit(127);
    }
    return 1;
#endif
}

/**
 * Stop one integration listener process.
 * @param process Process handle.
 * @return None.
 */
static void test_stop_listener(netl_test_process_t *process)
{
#ifdef _WIN32
    if (process->hProcess) {
        (void)TerminateProcess(process->hProcess, 0);
        (void)WaitForSingleObject(process->hProcess, 2000);
        CloseHandle(process->hProcess);
        process->hProcess = NULL;
    }
#else
    if (*process > 0) {
        (void)kill(*process, SIGTERM);
        (void)waitpid(*process, NULL, 0);
        *process = 0;
    }
#endif
}

/**
 * Connect to the local integration listener with retries.
 * @param port Listener port.
 * @return Connected socket, or invalid socket on failure.
 */
static netl_test_fd_t test_connect(unsigned short port)
{
    struct sockaddr_in address;
    int attempt;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);

    for (attempt = 0; attempt < 50; attempt++) {
        netl_test_fd_t fd;

        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == NETL_TEST_INVALID) return NETL_TEST_INVALID;
        if (connect(
                fd,
                (const struct sockaddr *)&address,
                (socklen_t)sizeof(address)
            ) == 0) {
            return fd;
        }
        NETL_TEST_CLOSE(fd);
        test_sleep(20UL);
    }
    return NETL_TEST_INVALID;
}

/**
 * Validate the real TCP listener and command bridge.
 * @return Nonzero on success.
 */
static int test_tcp_dispatch(void)
{
    unsigned short port;
    netl_test_process_t process;
    netl_test_fd_t fd;
    int ok;

#ifdef _WIN32
    memset(&process, 0, sizeof(process));
#else
    process = 0;
#endif
    port = test_port();
    if (port == 0U || !test_start_listener(port, &process)) return 0;
    fd = test_connect(port);
    if (fd == NETL_TEST_INVALID) {
        test_stop_listener(&process);
        return 0;
    }

    ok = send(fd, "ping\n", 5, 0) == 5;
#ifndef _WIN32
    if (ok) {
        char buffer[5];
        size_t used;

        used = 0U;
        while (used < sizeof(buffer)) {
            ssize_t amount;

            amount = recv(fd, buffer + used, sizeof(buffer) - used, 0);
            if (amount <= 0) {
                ok = 0;
                break;
            }
            used += (size_t)amount;
        }
        if (ok && memcmp(buffer, "ping\n", sizeof(buffer)) != 0) ok = 0;
    }
#endif
    NETL_TEST_CLOSE(fd);
    test_stop_listener(&process);
    return ok;
}

/**
 * Run all netl CLI contract tests.
 * @return Process status.
 */
int main(void)
{
#ifdef _WIN32
    WSADATA data;

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 1;
#endif
    test_current = 0;
    test_failures = 0;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing", "missing port fails", run_cli("") != 0);
    result("command", "missing command fails", run_cli("8080") != 0);
    result("unknown", "unknown option fails",
        run_cli("8080 true --unknown") != 0);
    result("port", "invalid port fails", run_cli("70000 true") != 0);
    result("transport", "invalid transport fails",
        run_cli("8080 true -t raw") != 0);
    result("transport value", "missing transport value fails",
        run_cli("8080 true -t") != 0);
    result("tcp", "TCP listener dispatches to the configured command",
        test_tcp_dispatch());

    printf("\n%d passed, %d failed\n",
        NETL_TEST_COUNT - test_failures,
        test_failures);
#ifdef _WIN32
    WSACleanup();
#endif
    return test_failures ? 1 : 0;
}
