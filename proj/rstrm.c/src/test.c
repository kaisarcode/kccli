/**
 * test.c - rstrm CLI contract tests.
 * Summary: Exercises argument parsing and full-duplex loopback stream relays.
 *
 * Author: KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU Lesser General Public License v3.0
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "rstrm"
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
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
typedef pthread_t test_thread_t;
typedef int test_socket_t;
typedef socklen_t test_socklen_t;
#define TEST_INVALID_SOCKET (-1)
#define test_socket_close close
#define DEV_NULL "/dev/null"
#endif

#define RSTRM_TEST_COUNT 18

typedef struct {
    char command[4096];
    int status;
} test_command_t;

static int test_current;
static int test_failures;

/**
 * Prints one test result.
 * @param name Test name.
 * @param detail Test detail.
 * @param ok Nonzero when the test passed.
 * @return void.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_current++;
    if (!ok) test_failures++;
    printf(
        "[%d/%d] [%s] %s: %s\n",
        test_current,
        RSTRM_TEST_COUNT,
        ok ? "PASS" : "FAIL",
        name,
        detail
    );
}

/**
 * Runs the CLI with one argument string.
 * @param args Command arguments.
 * @return Shell exit status.
 */
static int run(const char *args)
{
    char command[4096];
    int count;

    count = snprintf(
        command,
        sizeof(command),
        "\"%s\" %s > " DEV_NULL " 2> " DEV_NULL,
        KC_CLI_PATH,
        args
    );
    if (count < 0 || (size_t)count >= sizeof(command)) return 1;
    return system(command);
}

/**
 * Starts the socket runtime when required.
 * @return Nonzero on success.
 */
static int socket_runtime_start(void)
{
#ifdef _WIN32
    WSADATA data;

    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return 1;
#endif
}

/**
 * Stops the socket runtime when required.
 * @return void.
 */
static void socket_runtime_stop(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * Obtains one unused loopback UDP port from the OS.
 * @return Port number, or zero on failure.
 */
static unsigned short ephemeral_port(void)
{
    test_socket_t socket_fd;
    struct sockaddr_in address;
    test_socklen_t address_size;
    unsigned short port;

    if (!socket_runtime_start()) return 0;
    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd == TEST_INVALID_SOCKET) {
        socket_runtime_stop();
        return 0;
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    address_size = (test_socklen_t)sizeof(address);
    port = 0;
    if (
        bind(
            socket_fd,
            (const struct sockaddr *)&address,
            address_size
        ) == 0 &&
        getsockname(
            socket_fd,
            (struct sockaddr *)&address,
            &address_size
        ) == 0
    ) {
        port = ntohs(address.sin_port);
    }
    test_socket_close(socket_fd);
    socket_runtime_stop();
    return port;
}

/**
 * Obtains two distinct loopback UDP ports.
 * @param first First port.
 * @param second Second port.
 * @return Nonzero on success.
 */
static int test_ports(unsigned short *first, unsigned short *second)
{
    unsigned int attempt;

    for (attempt = 0U; attempt < 32U; attempt++) {
        *first = ephemeral_port();
        *second = ephemeral_port();
        if (*first && *second && *first != *second) return 1;
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
    test_command_t *command;

    command = (test_command_t *)arg;
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
static int start_command_thread(
    test_thread_t *thread,
    test_command_t *command
)
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
 * @return void.
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
 * Sleeps briefly between peer starts.
 * @param milliseconds Delay duration.
 * @return void.
 */
static void test_sleep(unsigned int milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec delay;

    delay.tv_sec = (time_t)(milliseconds / 1000U);
    delay.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
    (void)nanosleep(&delay, NULL);
#endif
}

/**
 * Writes a deterministic binary payload.
 * @param path Output path.
 * @param size Payload size.
 * @param seed Payload variant seed.
 * @return Nonzero on success.
 */
static int write_payload(const char *path, size_t size, unsigned int seed)
{
    unsigned char buffer[4096];
    FILE *file;
    size_t offset;

    file = fopen(path, "wb");
    if (!file) return 0;
    offset = 0U;
    while (offset < size) {
        size_t part;
        size_t index;

        part = size - offset;
        if (part > sizeof(buffer)) part = sizeof(buffer);
        for (index = 0U; index < part; index++) {
            buffer[index] = (unsigned char)(
                ((offset + index) * 37U + (size_t)seed) & 0xffU
            );
        }
        if (fwrite(buffer, 1, part, file) != part) {
            fclose(file);
            return 0;
        }
        offset += part;
    }
    return fclose(file) == 0;
}

/**
 * Compares two files byte-for-byte.
 * @param first First file.
 * @param second Second file.
 * @return Nonzero when the files are identical.
 */
static int files_equal(const char *first, const char *second)
{
    FILE *a;
    FILE *b;
    int equal;
    int ca;
    int cb;

    a = fopen(first, "rb");
    b = fopen(second, "rb");
    if (!a || !b) {
        if (a) fclose(a);
        if (b) fclose(b);
        return 0;
    }
    equal = 1;
    do {
        ca = fgetc(a);
        cb = fgetc(b);
        if (ca != cb) equal = 0;
    } while (equal && ca != EOF && cb != EOF);
    fclose(a);
    fclose(b);
    return equal;
}

/**
 * Checks whether one file is empty.
 * @param path File path.
 * @return Nonzero when the file contains no bytes.
 */
static int file_is_empty(const char *path)
{
    FILE *file;
    int value;

    file = fopen(path, "rb");
    if (!file) return 0;
    value = fgetc(file);
    fclose(file);
    return value == EOF;
}

/**
 * Relays one large binary payload after the peer half-closes first.
 * @return Nonzero on success.
 */
static int one_way_test(void)
{
    unsigned short sender_port;
    unsigned short receiver_port;
    test_command_t receiver;
    test_thread_t receiver_thread;
    char sender[4096];
    int sender_status;

    if (!test_ports(&sender_port, &receiver_port)) return 0;
    if (!write_payload("rstrm-test-one.bin", 262161U, 17U)) return 0;
    memset(&receiver, 0, sizeof(receiver));
    receiver.status = 1;
    snprintf(
        receiver.command,
        sizeof(receiver.command),
        "\"%s\" 127.0.0.1 %u --host 127.0.0.1 -p %u "
        "< " DEV_NULL " > rstrm-test-one.out 2> rstrm-test-one-b.err",
        KC_CLI_PATH,
        (unsigned)sender_port,
        (unsigned)receiver_port
    );
    if (!start_command_thread(&receiver_thread, &receiver)) return 0;
    test_sleep(100U);
    snprintf(
        sender,
        sizeof(sender),
        "\"%s\" 127.0.0.1 %u --host 127.0.0.1 -p %u --mtu 1200 "
        "< rstrm-test-one.bin > rstrm-test-empty.out "
        "2> rstrm-test-one-a.err",
        KC_CLI_PATH,
        (unsigned)receiver_port,
        (unsigned)sender_port
    );
    sender_status = system(sender);
    join_command_thread(receiver_thread);
    return sender_status == 0 && receiver.status == 0 &&
        files_equal("rstrm-test-one.bin", "rstrm-test-one.out") &&
        file_is_empty("rstrm-test-empty.out");
}

/**
 * Relays simultaneous binary payloads in both directions.
 * @return Nonzero on success.
 */
static int full_duplex_test(void)
{
    unsigned short first_port;
    unsigned short second_port;
    test_command_t first;
    test_command_t second;
    test_thread_t first_thread;
    test_thread_t second_thread;

    if (!test_ports(&first_port, &second_port)) return 0;
    if (!write_payload("rstrm-test-a.bin", 131101U, 29U)) return 0;
    if (!write_payload("rstrm-test-b.bin", 147509U, 83U)) return 0;
    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    first.status = 1;
    second.status = 1;
    snprintf(
        first.command,
        sizeof(first.command),
        "\"%s\" 127.0.0.1 %u -p %u < rstrm-test-a.bin "
        "> rstrm-test-a.out 2> rstrm-test-a.err",
        KC_CLI_PATH,
        (unsigned)second_port,
        (unsigned)first_port
    );
    snprintf(
        second.command,
        sizeof(second.command),
        "\"%s\" 127.0.0.1 %u -p %u < rstrm-test-b.bin "
        "> rstrm-test-b.out 2> rstrm-test-b.err",
        KC_CLI_PATH,
        (unsigned)first_port,
        (unsigned)second_port
    );
    if (!start_command_thread(&first_thread, &first)) return 0;
    if (!start_command_thread(&second_thread, &second)) {
        join_command_thread(first_thread);
        return 0;
    }
    join_command_thread(first_thread);
    join_command_thread(second_thread);
    return first.status == 0 && second.status == 0 &&
        files_equal("rstrm-test-b.bin", "rstrm-test-a.out") &&
        files_equal("rstrm-test-a.bin", "rstrm-test-b.out");
}

/**
 * Removes files produced by integration tests.
 * @return void.
 */
static void cleanup(void)
{
    remove("rstrm-test-one.bin");
    remove("rstrm-test-one.out");
    remove("rstrm-test-empty.out");
    remove("rstrm-test-one-a.err");
    remove("rstrm-test-one-b.err");
    remove("rstrm-test-a.bin");
    remove("rstrm-test-b.bin");
    remove("rstrm-test-a.out");
    remove("rstrm-test-b.out");
    remove("rstrm-test-a.err");
    remove("rstrm-test-b.err");
}

/**
 * Runs all CLI contract and loopback integration tests.
 * @return Zero when every test passes.
 */
int main(void)
{
    test_current = 0;
    test_failures = 0;

    result("help", "--help succeeds", run("--help") == 0);
    result("short help", "-h succeeds", run("-h") == 0);
    result("version", "--version succeeds", run("--version") == 0);
    result("short version", "-v succeeds", run("-v") == 0);
    result("missing peer", "empty invocation fails", run("") != 0);
    result(
        "missing peer port",
        "peer host without a port fails",
        run("127.0.0.1") != 0
    );
    result(
        "invalid peer port",
        "non-numeric peer port fails",
        run("127.0.0.1 invalid") != 0
    );
    result(
        "zero peer port",
        "peer port zero fails",
        run("127.0.0.1 0") != 0
    );
    result(
        "invalid local port",
        "local port range is validated",
        run("127.0.0.1 9 -p 70000") != 0
    );
    result(
        "host value",
        "--host requires a following value",
        run("127.0.0.1 9 --host") != 0
    );
    result(
        "port value",
        "--port requires a following value",
        run("127.0.0.1 9 --port") != 0
    );
    result(
        "mtu value",
        "--mtu requires a following value",
        run("127.0.0.1 9 --mtu") != 0
    );
    result(
        "invalid mtu text",
        "non-numeric MTU fails",
        run("127.0.0.1 9 --mtu invalid") != 0
    );
    result(
        "invalid mtu range",
        "library MTU validation is surfaced",
        run("127.0.0.1 9 --mtu 1") != 0
    );
    result(
        "unknown option",
        "unknown options fail",
        run("127.0.0.1 9 --wat") != 0
    );
    result(
        "extra positional",
        "extra endpoint arguments fail",
        run("127.0.0.1 9 extra") != 0
    );
    result(
        "one way",
        "large binary data survives peer-first half-close",
        one_way_test()
    );
    result(
        "full duplex",
        "simultaneous binary streams complete exactly",
        full_duplex_test()
    );

    cleanup();
    printf(
        "\n%d passed, %d failed\n",
        RSTRM_TEST_COUNT - test_failures,
        test_failures
    );
    return test_failures == 0 ? 0 : 1;
}
