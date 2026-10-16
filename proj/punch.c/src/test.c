/**
 * test.c - punch CLI contract tests.
 * Summary: Exercises argument validation and one-datagram loopback exchange.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "punch"
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef HANDLE test_thread_t;
typedef SOCKET test_socket_t;
typedef int test_socklen_t;
#define TEST_INVALID_SOCKET INVALID_SOCKET
#define test_socket_close closesocket
#define DEV_NULL "NUL"
#define CAT_FILE "type"
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
typedef pthread_t test_thread_t;
typedef int test_socket_t;
typedef socklen_t test_socklen_t;
#define TEST_INVALID_SOCKET (-1)
#define test_socket_close close
#define DEV_NULL "/dev/null"
#define CAT_FILE "cat"
#endif

#define PUNCH_TEST_COUNT 16

static int test_current;
static int test_failures;

typedef struct {
    char command[4096];
    int status;
} test_command_t;

/**
 * Prints one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Non-zero when the test passed.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf("[%d/%d] [%s] %s: %s\n", test_current, PUNCH_TEST_COUNT,
        ok ? "PASS" : "FAIL", name, detail);
}

/**
 * Runs the CLI with one argument string.
 * @param args Command arguments.
 * @return Shell exit status.
 */
static int run(const char *args)
{
    char command[4096];
    int n = snprintf(command, sizeof(command), "%s %s", KC_CLI_PATH, args);
    if (n < 0 || (size_t)n >= sizeof(command)) return 1;
    return system(command);
}

/**
 * Starts the socket runtime when required.
 * @return Nonzero on success.
 */
static int socket_runtime_start(void)
{
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return 1;
#endif
}

/**
 * Stops the socket runtime when required.
 * @return None.
 */
static void socket_runtime_stop(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * Obtains an unused loopback UDP port from the OS.
 * @return Port number, or zero on failure.
 */
static unsigned short ephemeral_port(void)
{
    test_socket_t fd = TEST_INVALID_SOCKET;
    struct sockaddr_in addr;
    test_socklen_t addr_len = (test_socklen_t)sizeof(addr);
    unsigned short port = 0;

    if (!socket_runtime_start()) return 0;
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == TEST_INVALID_SOCKET) goto done;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(fd, (const struct sockaddr *)&addr, addr_len) != 0) goto done;
    if (getsockname(fd, (struct sockaddr *)&addr, &addr_len) != 0) goto done;
    port = ntohs(addr.sin_port);

done:
    if (fd != TEST_INVALID_SOCKET) test_socket_close(fd);
    socket_runtime_stop();
    return port;
}

/**
 * Obtains distinct ports for two peers and one dead candidate.
 * @param a First peer port.
 * @param b Second peer port.
 * @param dead Dead candidate port.
 * @return Nonzero on success.
 */
static int test_ports(unsigned short *a, unsigned short *b,
    unsigned short *dead)
{
    unsigned attempt;
    for (attempt = 0; attempt < 16U; ++attempt) {
        *a = ephemeral_port();
        *b = ephemeral_port();
        *dead = ephemeral_port();
        if (*a && *b && *dead && *a != *b && *a != *dead && *b != *dead)
            return 1;
    }
    return 0;
}

#ifdef _WIN32
/**
 * Runs one shell command in a Windows test thread.
 * @param arg Test command.
 * @return Zero.
 */
static DWORD WINAPI command_thread(LPVOID arg)
#else
/**
 * Runs one shell command in a POSIX test thread.
 * @param arg Test command.
 * @return NULL.
 */
static void *command_thread(void *arg)
#endif
{
    test_command_t *command = (test_command_t *)arg;
    command->status = system(command->command);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/**
 * Starts one command thread.
 * @param thread Output thread handle.
 * @param command Command state.
 * @return Nonzero on success.
 */
static int start_command_thread(test_thread_t *thread, test_command_t *command)
{
#ifdef _WIN32
    *thread = CreateThread(NULL, 0, command_thread, command, 0, NULL);
    return *thread != NULL;
#else
    return pthread_create(thread, NULL, command_thread, command) == 0;
#endif
}

/**
 * Waits for one command thread.
 * @param thread Thread handle.
 * @return None.
 */
static void join_command_thread(test_thread_t thread)
{
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
}

/**
 * Writes a binary test datagram.
 * @param path Output path.
 * @param variant Payload variant.
 * @return Nonzero on success.
 */
static int write_payload(const char *path, int variant)
{
    static const unsigned char first[] = { 'p', 'u', 'n', 'c', 'h', 0, 'x' };
    static const unsigned char second[] = { 1, 2, 3, 4, 5, 0, 7 };
    const unsigned char *data = variant ? second : first;
    size_t size = variant ? sizeof(second) : sizeof(first);
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    if (fwrite(data, 1, size, file) != size) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

/**
 * Compares two files byte-for-byte.
 * @param a First file.
 * @param b Second file.
 * @return Nonzero when identical.
 */
static int files_equal(const char *a, const char *b)
{
    FILE *fa = fopen(a, "rb");
    FILE *fb = fopen(b, "rb");
    int equal = 1;
    int ca;
    int cb;

    if (!fa || !fb) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        return 0;
    }
    do {
        ca = fgetc(fa);
        cb = fgetc(fb);
        if (ca != cb) equal = 0;
    } while (equal && ca != EOF && cb != EOF);
    fclose(fa);
    fclose(fb);
    return equal;
}

/**
 * Runs one real send/receive pair using a dead first candidate.
 * @return Nonzero on success.
 */
static int exchange_test(void)
{
    unsigned short send_port;
    unsigned short receive_port;
    unsigned short dead_port;
    test_command_t receiver;
    test_thread_t receiver_thread;
    char sender[4096];
    int sender_status;

    if (!test_ports(&send_port, &receive_port, &dead_port)) return 0;
    if (!write_payload("punch-test-input.bin", 0)) return 0;
    memset(&receiver, 0, sizeof(receiver));
    snprintf(receiver.command, sizeof(receiver.command),
        "%s receive 127.0.0.1:%u -p %u -t 2000 --receive-timeout 2000 "
        "> punch-test-output.bin 2> punch-test-receive.err",
        KC_CLI_PATH, (unsigned)send_port, (unsigned)receive_port);
    if (!start_command_thread(&receiver_thread, &receiver)) return 0;
    snprintf(sender, sizeof(sender),
        "%s send 127.0.0.1:%u 127.0.0.1:%u -p %u -t 2000 "
        "-i punch-test-input.bin > " DEV_NULL " 2> punch-test-send.err",
        KC_CLI_PATH, (unsigned)dead_port, (unsigned)receive_port,
        (unsigned)send_port);
    sender_status = system(sender);
    join_command_thread(receiver_thread);
    return sender_status == 0 && receiver.status == 0 &&
        files_equal("punch-test-input.bin", "punch-test-output.bin");
}

/**
 * Verifies that stdin takes precedence over --input for send.
 * @return Nonzero on success.
 */
static int stdin_precedence_test(void)
{
    unsigned short send_port;
    unsigned short receive_port;
    unsigned short dead_port;
    test_command_t receiver;
    test_thread_t receiver_thread;
    char sender[4096];
    int sender_status;

    if (!test_ports(&send_port, &receive_port, &dead_port)) return 0;
    if (!write_payload("punch-test-stdin.bin", 0) ||
        !write_payload("punch-test-file.bin", 1)) return 0;
    memset(&receiver, 0, sizeof(receiver));
    snprintf(receiver.command, sizeof(receiver.command),
        "%s receive 127.0.0.1:%u -p %u -t 2000 --receive-timeout 2000 "
        "> punch-test-precedence.bin 2> punch-test-receive.err",
        KC_CLI_PATH, (unsigned)send_port, (unsigned)receive_port);
    if (!start_command_thread(&receiver_thread, &receiver)) return 0;
    snprintf(sender, sizeof(sender),
        CAT_FILE " punch-test-stdin.bin | %s send 127.0.0.1:%u -p %u -t 2000 "
        "-i punch-test-file.bin > " DEV_NULL " 2> punch-test-send.err",
        KC_CLI_PATH, (unsigned)receive_port, (unsigned)send_port);
    sender_status = system(sender);
    join_command_thread(receiver_thread);
    return sender_status == 0 && receiver.status == 0 &&
        files_equal("punch-test-stdin.bin", "punch-test-precedence.bin");
}

/**
 * Test entry point.
 * @return Exit status.
 */
int main(void)
{
    test_current = 0;
    test_failures = 0;

    result("help", "--help succeeds", run("--help > " DEV_NULL) == 0);
    result("version", "--version succeeds", run("--version > " DEV_NULL) == 0);
    result("missing", "missing command fails",
        run("> " DEV_NULL " 2>&1") != 0);
    result("unknown-command", "unknown command fails",
        run("wat 127.0.0.1:1 > " DEV_NULL " 2>&1") != 0);
    result("unknown-option", "unknown option fails",
        run("receive 127.0.0.1:1 --wat > " DEV_NULL " 2>&1") != 0);
    result("send-peer", "send requires a peer",
        run("send < " DEV_NULL " > " DEV_NULL " 2>&1") != 0);
    result("receive-peer", "receive requires a peer",
        run("receive > " DEV_NULL " 2>&1") != 0);
    result("peer-format", "peer requires host and port",
        run("receive localhost > " DEV_NULL " 2>&1") != 0);
    result("peer-ipv6", "unbracketed IPv6 is rejected",
        run("receive ::1:1234 > " DEV_NULL " 2>&1") != 0);
    result("peer-port", "peer port range is validated",
        run("receive 127.0.0.1:70000 > " DEV_NULL " 2>&1") != 0);
    result("local-port", "local port range is validated",
        run("receive 127.0.0.1:1 -p 70000 > " DEV_NULL " 2>&1") != 0);
    result("timeout", "connection timeout is validated",
        run("receive 127.0.0.1:1 -t nope > " DEV_NULL " 2>&1") != 0);
    result("send-options", "send rejects receive-only timeout",
        run("send 127.0.0.1:1 --receive-timeout 1 < " DEV_NULL
            " > " DEV_NULL " 2>&1") != 0);
    result("receive-options", "receive rejects send input option",
        run("receive 127.0.0.1:1 -i input.bin > " DEV_NULL " 2>&1") != 0);
    result("exchange", "sends and receives one binary datagram", exchange_test());
    result("stdin-precedence", "stdin wins over --input", stdin_precedence_test());

    remove("punch-test-send.err");
    remove("punch-test-receive.err");
    remove("punch-test-precedence.bin");
    remove("punch-test-file.bin");
    remove("punch-test-stdin.bin");
    remove("punch-test-output.bin");
    remove("punch-test-input.bin");

    printf("\n%d passed, %d failed\n",
        PUNCH_TEST_COUNT - test_failures, test_failures);
    return test_failures ? 1 : 0;
}
