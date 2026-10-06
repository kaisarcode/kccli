/**
 * test.c - netl CLI contract tests.
 * Summary: Validates resident listener administration and network behavior.
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
#include <sys/socket.h>
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
 * Execute the CLI and capture standard output.
 * @param args Arguments appended after the executable.
 * @param buffer Output buffer.
 * @param capacity Buffer capacity.
 * @return Nonzero on success.
 */
static int capture_output(
    const char *args,
    char *buffer,
    size_t capacity
)
{
    const char *path = "netl-cli-output.txt";
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
        path
    );
#else
    snprintf(
        command,
        sizeof(command),
        "\"%s\" %s > %s 2>/dev/null",
        KC_CLI_PATH,
        args ? args : "",
        path
    );
#endif
    if (system(command) != 0) {
        (void)remove(path);
        return 0;
    }
    file = fopen(path, "rb");
    if (!file) return 0;
    size = fread(buffer, 1, capacity - 1U, file);
    fclose(file);
    (void)remove(path);
    buffer[size] = '\0';
    return 1;
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
 * Remove the isolated runtime directory.
 * @return None.
 */
static void clear_runtime(void)
{
#ifdef _WIN32
    (void)system("rmdir /S /Q netl-cli-runtime >NUL 2>&1");
#else
    (void)system("rm -rf netl-cli-runtime");
#endif
}

/**
 * Configure an isolated runtime directory.
 * @return Nonzero on success.
 */
static int configure_runtime(void)
{
#ifdef _WIN32
    return _putenv_s("KC_NETL_DIR", "netl-cli-runtime") == 0;
#else
    return setenv("KC_NETL_DIR", "netl-cli-runtime", 1) == 0;
#endif
}

/**
 * Reserve and release one local TCP port.
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

/**
 * Connect to one local TCP port with retries.
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
 * Check that one local TCP port no longer accepts connections.
 * @param port Listener port.
 * @return Nonzero when closed.
 */
static int test_closed(unsigned short port)
{
    struct sockaddr_in address;
    netl_test_fd_t fd;
    int rc;

    test_sleep(100UL);
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == NETL_TEST_INVALID) return 1;
    rc = connect(
        fd,
        (const struct sockaddr *)&address,
        (socklen_t)sizeof(address)
    );
    NETL_TEST_CLOSE(fd);
    return rc != 0;
}

/**
 * Run all netl CLI contract tests.
 * @return Process status.
 */
int main(void)
{
    char create_args[256];
    char expected[512];
    char output[1024];
    unsigned short port;
    netl_test_fd_t fd;
    int runtime_ok;
    int create_ok;
    int list_ok;
    int delete_ok;

#ifdef _WIN32
    WSADATA data;

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 1;
#endif
    test_current = 0;
    test_failures = 0;

    result("help", "--help succeeds", run_cli("--help") == 0);
    result("version", "--version succeeds", run_cli("--version") == 0);
    result("missing", "missing listener id fails", run_cli("") != 0);
    result(
        "command",
        "creation without command fails",
        run_cli("sample 8080") != 0
    );
    result(
        "unknown",
        "unknown option fails",
        run_cli("sample 8080 true --unknown") != 0
    );
    result(
        "bad id",
        "invalid listener id fails",
        run_cli("bad/name -d") != 0
    );

    runtime_ok = configure_runtime();
    clear_runtime();
    port = runtime_ok ? test_port() : 0U;
#ifdef _WIN32
    snprintf(
        create_args,
        sizeof(create_args),
        "sample %u more",
        (unsigned int)port
    );
    snprintf(
        expected,
        sizeof(expected),
        "sample\t%u\ttcp\tmore\n",
        (unsigned int)port
    );
#else
    snprintf(
        create_args,
        sizeof(create_args),
        "sample %u cat",
        (unsigned int)port
    );
    snprintf(
        expected,
        sizeof(expected),
        "sample\t%u\ttcp\tcat\n",
        (unsigned int)port
    );
#endif

    create_ok = runtime_ok && port != 0U && run_cli(create_args) == 0;
    fd = create_ok ? test_connect(port) : NETL_TEST_INVALID;
    if (fd != NETL_TEST_INVALID) NETL_TEST_CLOSE(fd);
    result(
        "create",
        "registers a resident TCP listener",
        create_ok && fd != NETL_TEST_INVALID
    );

    list_ok = capture_output("-l sample", output, sizeof(output)) &&
        strcmp(output, expected) == 0;
    result(
        "list",
        "list emits id, port, transport, and command",
        list_ok
    );

    delete_ok = run_cli("sample -d") == 0 && test_closed(port);
    result(
        "delete",
        "removes the resident listener",
        delete_ok
    );
    clear_runtime();

    printf(
        "\n%d passed, %d failed\n",
        NETL_TEST_COUNT - test_failures,
        test_failures
    );
#ifdef _WIN32
    WSACleanup();
#endif
    return test_failures ? 1 : 0;
}
