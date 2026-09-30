/**
 * redp2p.c - REDP2P command-line interface.
 * Summary: Thin CLI over the idx/pub/con public capability API.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: https://www.gnu.org/licenses/gpl-3.0.html
 */

#include "libredp2p.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdatomic.h>
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
typedef SOCKET redp2p_cli_fd_t;
typedef WSAPOLLFD redp2p_cli_pollfd_t;
#define REDP2P_CLI_POLLIN POLLRDNORM
#define REDP2P_CLI_FD_INVALID INVALID_SOCKET
#define REDP2P_CLI_FD_CLOSE(f) closesocket(f)
#define REDP2P_CLI_ISERR(f) ((f) == INVALID_SOCKET)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
typedef int redp2p_cli_fd_t;
typedef struct pollfd redp2p_cli_pollfd_t;
#define REDP2P_CLI_POLLIN POLLIN
#define REDP2P_CLI_FD_INVALID (-1)
#define REDP2P_CLI_FD_CLOSE(f) close(f)
#define REDP2P_CLI_ISERR(f) ((f) < 0)
#endif

#define REDP2P_CLI_BUF 4096

static volatile sig_atomic_t redp2p_cli_stop = 0;

/**
 * Records a CLI termination signal.
 * @param sig Signal number.
 * @return None.
 */
static void redp2p_cli_signal(int sig)
{
    (void)sig;
    redp2p_cli_stop = 1;
}

/**
 * Sleeps briefly while a long-running CLI command remains active.
 * @return None.
 */
static void redp2p_cli_sleep(void)
{
#ifdef _WIN32
    Sleep(250);
#else
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 250000000L;
    (void)nanosleep(&delay, NULL);
#endif
}

/**
 * Initializes CLI socket support.
 * @return 0 on success, nonzero on failure.
 */
static int redp2p_cli_platform_init(void)
{
#ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0 ? 0 : 1;
#else
    return 0;
#endif
}

/**
 * Releases CLI socket support.
 * @return None.
 */
static void redp2p_cli_platform_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * Reads bytes from one CLI socket.
 * @param fd Socket descriptor.
 * @param buffer Destination buffer.
 * @param size Destination capacity.
 * @return Received byte count, zero on close, or negative on failure.
 */
static int redp2p_cli_sock_read(redp2p_cli_fd_t fd, char *buffer, int size)
{
    return (int)recv(fd, buffer, size, 0);
}

/**
 * Writes an entire stream payload.
 * @param fd Socket descriptor.
 * @param buffer Source bytes.
 * @param size Byte count.
 * @return 0 on success, -1 on failure.
 */
static int redp2p_cli_write_all(redp2p_cli_fd_t fd, const char *buffer,
    int size)
{
    int off = 0;

    while (off < size) {
        int n = (int)send(fd, buffer + off, size - off, 0);
        if (n <= 0) return -1;
        off += n;
    }
    return 0;
}

/**
 * Waits for CLI socket readiness.
 * @param fds Poll descriptor array.
 * @param count Descriptor count.
 * @param timeout_ms Timeout in milliseconds.
 * @return Number of ready descriptors, zero on timeout, or negative on error.
 */
static int redp2p_cli_poll_wait(redp2p_cli_pollfd_t *fds, size_t count,
    int timeout_ms)
{
#ifdef _WIN32
    return WSAPoll(fds, (ULONG)count, timeout_ms);
#else
    return poll(fds, (nfds_t)count, timeout_ms);
#endif
}

/**
 * Reports whether one poll descriptor is readable.
 * @param fd Poll descriptor.
 * @return Nonzero when readable.
 */
static int redp2p_cli_poll_readable(const redp2p_cli_pollfd_t *fd)
{
    return fd && (fd->revents & REDP2P_CLI_POLLIN) != 0;
}

/**
 * Compares two local socket endpoints.
 * @param a First endpoint.
 * @param b Second endpoint.
 * @return Nonzero when equal.
 */
static int redp2p_cli_sockaddr_equal(const struct sockaddr_storage *a,
    const struct sockaddr_storage *b)
{
    if (!a || !b || a->ss_family != b->ss_family) return 0;
    if (a->ss_family == AF_INET) {
        const struct sockaddr_in *aa = (const struct sockaddr_in *)a;
        const struct sockaddr_in *bb = (const struct sockaddr_in *)b;
        return aa->sin_port == bb->sin_port &&
            aa->sin_addr.s_addr == bb->sin_addr.s_addr;
    }
    if (a->ss_family == AF_INET6) {
        const struct sockaddr_in6 *aa = (const struct sockaddr_in6 *)a;
        const struct sockaddr_in6 *bb = (const struct sockaddr_in6 *)b;
        return aa->sin6_port == bb->sin6_port &&
            memcmp(&aa->sin6_addr, &bb->sin6_addr,
                sizeof(aa->sin6_addr)) == 0;
    }
    return 0;
}

/**
 * Parses one nonzero 16-bit unsigned integer.
 * @param text Input text.
 * @param out Destination value.
 * @return 1 on success, 0 on invalid input.
 */
static int redp2p_cli_u16(const char *text, uint16_t *out)
{
    unsigned long value;
    char *end;

    if (!text || !text[0] || !out) return 0;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno || *end != '\0' || value == 0 || value > 65535) return 0;
    *out = (uint16_t)value;
    return 1;
}

/**
 * Parses one nonnegative size value.
 * @param text Input text.
 * @param out Destination value.
 * @return 1 on success, 0 on invalid input.
 */
static int redp2p_cli_size(const char *text, size_t *out)
{
    unsigned long long value;
    char *end;

    if (!text || !text[0] || !out) return 0;
    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno || *end != '\0' || value > SIZE_MAX) return 0;
    *out = (size_t)value;
    return 1;
}

/**
 * Parses one bounded unsigned integer.
 * @param text Input text.
 * @param max Maximum accepted value.
 * @param out Destination value.
 * @return 1 on success, 0 on invalid input.
 */
static int redp2p_cli_uint(const char *text, unsigned int max,
    unsigned int *out)
{
    unsigned long value;
    char *end;

    if (!text || !text[0] || !out) return 0;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno || *end != '\0' || value > max) return 0;
    *out = (unsigned int)value;
    return 1;
}

/**
 * Parses one publisher specification in id@index form.
 * @param text Specification text.
 * @param id Destination publisher identifier.
 * @param index Destination index endpoint.
 * @return 1 on success, 0 on invalid input.
 */
static int redp2p_cli_spec(const char *text, char id[KC_REDP2P_ID_MAX + 1],
    char index[320])
{
    const char *at;
    size_t id_len;
    size_t index_len;

    if (!text || !id || !index) return 0;
    at = strchr(text, '@');
    if (!at || at == text || !at[1]) return 0;
    id_len = (size_t)(at - text);
    index_len = strlen(at + 1);
    if (id_len > KC_REDP2P_ID_MAX || index_len >= 320) return 0;
    memcpy(id, text, id_len);
    id[id_len] = '\0';
    memcpy(index, at + 1, index_len + 1);
    return 1;
}

#ifdef _WIN32
typedef HANDLE redp2p_cli_thread_t;
typedef CRITICAL_SECTION redp2p_cli_mutex_t;
#define REDP2P_CLI_THREAD(name) static DWORD WINAPI name(LPVOID arg)
#define REDP2P_CLI_THREAD_RETURN() return 0
#else
typedef pthread_t redp2p_cli_thread_t;
typedef pthread_mutex_t redp2p_cli_mutex_t;
#define REDP2P_CLI_THREAD(name) static void *name(void *arg)
#define REDP2P_CLI_THREAD_RETURN() return NULL
#endif

typedef struct redp2p_cli_pub_client redp2p_cli_pub_client_t;
typedef struct redp2p_cli_con_tcp_session redp2p_cli_con_tcp_session_t;
typedef struct redp2p_cli_con_udp_session redp2p_cli_con_udp_session_t;

typedef struct {
    int protocol;
    uint16_t port;
    _Atomic int stopping;
    redp2p_cli_mutex_t mutex;
    redp2p_cli_pub_client_t *clients;
} redp2p_cli_pub_state_t;

struct redp2p_cli_pub_client {
    redp2p_cli_pub_state_t *owner;
    kc_redp2p_client_t *client;
    redp2p_cli_fd_t fd;
    redp2p_cli_thread_t thread;
    int thread_started;
    redp2p_cli_pub_client_t *next;
};

typedef struct {
    const char *id;
    const char *index;
    const char *stun;
    const char *turn;
    const char *turn_user;
    const char *turn_pass;
    _Atomic int stopping;
    redp2p_cli_mutex_t mutex;
    redp2p_cli_con_tcp_session_t *sessions;
} redp2p_cli_con_tcp_state_t;

struct redp2p_cli_con_tcp_session {
    redp2p_cli_con_tcp_state_t *owner;
    redp2p_cli_fd_t fd;
    kc_redp2p_con_t *con;
    redp2p_cli_thread_t thread;
    int thread_started;
    redp2p_cli_con_tcp_session_t *next;
};

typedef struct {
    redp2p_cli_fd_t fd;
    const char *id;
    const char *index;
    const char *stun;
    const char *turn;
    const char *turn_user;
    const char *turn_pass;
    _Atomic int stopping;
    redp2p_cli_con_udp_session_t *sessions;
} redp2p_cli_con_udp_state_t;

struct redp2p_cli_con_udp_session {
    redp2p_cli_con_udp_state_t *owner;
    struct sockaddr_storage address;
    socklen_t address_len;
    kc_redp2p_con_t *con;
    redp2p_cli_con_udp_session_t *next;
};

/**
 * Initializes one CLI synchronization mutex.
 * @param mutex Mutex to initialize.
 * @return 0 on success, nonzero on failure.
 */
static int redp2p_cli_mutex_init(redp2p_cli_mutex_t *mutex)
{
#ifdef _WIN32
    InitializeCriticalSection(mutex);
    return 0;
#else
    return pthread_mutex_init(mutex, NULL);
#endif
}

/**
 * Locks one CLI synchronization mutex.
 * @param mutex Mutex to lock.
 * @return None.
 */
static void redp2p_cli_mutex_lock(redp2p_cli_mutex_t *mutex)
{
#ifdef _WIN32
    EnterCriticalSection(mutex);
#else
    pthread_mutex_lock(mutex);
#endif
}

/**
 * Unlocks one CLI synchronization mutex.
 * @param mutex Mutex to unlock.
 * @return None.
 */
static void redp2p_cli_mutex_unlock(redp2p_cli_mutex_t *mutex)
{
#ifdef _WIN32
    LeaveCriticalSection(mutex);
#else
    pthread_mutex_unlock(mutex);
#endif
}

/**
 * Destroys one CLI synchronization mutex.
 * @param mutex Mutex to destroy.
 * @return None.
 */
static void redp2p_cli_mutex_destroy(redp2p_cli_mutex_t *mutex)
{
#ifdef _WIN32
    DeleteCriticalSection(mutex);
#else
    pthread_mutex_destroy(mutex);
#endif
}

#ifdef _WIN32
/**
 * Starts one Windows CLI worker thread.
 * @param thread Destination thread handle.
 * @param fn Worker entry point.
 * @param arg Worker argument.
 * @return 0 on success, -1 on failure.
 */
static int redp2p_cli_thread_start(redp2p_cli_thread_t *thread,
    LPTHREAD_START_ROUTINE fn, void *arg)
{
    *thread = CreateThread(NULL, 0, fn, arg, 0, NULL);
    return *thread ? 0 : -1;
}
#else
/**
 * Starts one POSIX CLI worker thread.
 * @param thread Destination thread handle.
 * @param fn Worker entry point.
 * @param arg Worker argument.
 * @return 0 on success, nonzero on failure.
 */
static int redp2p_cli_thread_start(redp2p_cli_thread_t *thread,
    void *(*fn)(void *), void *arg)
{
    return pthread_create(thread, NULL, fn, arg);
}
#endif

/**
 * Joins one CLI worker thread.
 * @param thread Thread to join.
 * @return None.
 */
static void redp2p_cli_thread_join(redp2p_cli_thread_t thread)
{
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
}

/**
 * Shuts down both directions of one CLI socket.
 * @param fd Socket descriptor.
 * @return None.
 */
static void redp2p_cli_socket_shutdown(redp2p_cli_fd_t fd)
{
    if (REDP2P_CLI_ISERR(fd)) return;
#ifdef _WIN32
    shutdown(fd, SD_BOTH);
#else
    shutdown(fd, SHUT_RDWR);
#endif
}

/**
 * Opens one CLI socket connected to a loopback service.
 * @param protocol REDP2P TCP or UDP protocol.
 * @param port Loopback service port.
 * @return Connected descriptor, or REDP2P_CLI_FD_INVALID on failure.
 */
static redp2p_cli_fd_t redp2p_cli_connect_local(int protocol, uint16_t port)
{
    struct sockaddr_in address;
    redp2p_cli_fd_t fd;
    int type;

    type = protocol == KC_REDP2P_TCP ? SOCK_STREAM : SOCK_DGRAM;
    fd = socket(AF_INET, type, 0);
    if (REDP2P_CLI_ISERR(fd)) return REDP2P_CLI_FD_INVALID;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(0x7f000001u);
    if (connect(fd, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        REDP2P_CLI_FD_CLOSE(fd);
        return REDP2P_CLI_FD_INVALID;
    }
    return fd;
}

/**
 * Opens one CLI loopback listener or datagram socket.
 * @param protocol REDP2P TCP or UDP protocol.
 * @param port Local loopback port.
 * @return Bound descriptor, or REDP2P_CLI_FD_INVALID on failure.
 */
static redp2p_cli_fd_t redp2p_cli_bind_local(int protocol, uint16_t port)
{
    struct sockaddr_in address;
    redp2p_cli_fd_t fd;
    int reuse;
    int type;

    type = protocol == KC_REDP2P_TCP ? SOCK_STREAM : SOCK_DGRAM;
    fd = socket(AF_INET, type, 0);
    if (REDP2P_CLI_ISERR(fd)) return REDP2P_CLI_FD_INVALID;
    reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
        sizeof(reuse));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(0x7f000001u);
    if (bind(fd, (const struct sockaddr *)&address, sizeof(address)) != 0 ||
        (protocol == KC_REDP2P_TCP && listen(fd, 64) != 0))
    {
        REDP2P_CLI_FD_CLOSE(fd);
        return REDP2P_CLI_FD_INVALID;
    }
    return fd;
}

/**
 * Writes one application payload to a CLI forwarding socket.
 * @param fd Destination socket.
 * @param protocol REDP2P TCP or UDP protocol.
 * @param data Application bytes.
 * @param size Byte count.
 * @return 0 on success, -1 on failure.
 */
static int redp2p_cli_socket_send(redp2p_cli_fd_t fd, int protocol,
    const void *data, size_t size)
{
    const unsigned char *cursor = (const unsigned char *)data;

    if (protocol == KC_REDP2P_UDP) {
        int sent;
        if (size > INT_MAX) return -1;
        sent = (int)send(fd, (const char *)data, (int)size, 0);
        return sent == (int)size ? 0 : -1;
    }
    while (size > 0) {
        int chunk = size > (size_t)INT_MAX ? INT_MAX : (int)size;
        if (redp2p_cli_write_all(fd, (const char *)cursor, chunk) != 0)
            return -1;
        cursor += (size_t)chunk;
        size -= (size_t)chunk;
    }
    return 0;
}

/**
 * Pumps one local publisher backend socket back into its peer channel.
 * @param arg Publisher client forwarding state.
 * @return Platform thread result.
 */
REDP2P_CLI_THREAD(redp2p_cli_pub_backend_worker)
{
    redp2p_cli_pub_client_t *state = (redp2p_cli_pub_client_t *)arg;
    unsigned char buffer[REDP2P_CLI_BUF];

    while (!atomic_load(&state->owner->stopping)) {
        int n = redp2p_cli_sock_read(state->fd, (char *)buffer,
            (int)sizeof(buffer));
        if (n < 0 || (n == 0 && state->owner->protocol == KC_REDP2P_TCP))
            break;
        if (kc_redp2p_client_respond(state->client, buffer, (size_t)n) !=
            KC_REDP2P_OK)
            break;
    }
    redp2p_cli_mutex_lock(&state->owner->mutex);
    if (!REDP2P_CLI_ISERR(state->fd)) {
        REDP2P_CLI_FD_CLOSE(state->fd);
        state->fd = REDP2P_CLI_FD_INVALID;
    }
    redp2p_cli_mutex_unlock(&state->owner->mutex);
    kc_redp2p_client_close(state->client);
    REDP2P_CLI_THREAD_RETURN();
}

/**
 * Attaches one established peer channel to the CLI publisher backend.
 * @param client Established publisher client.
 * @param userdata Publisher CLI state.
 * @return None.
 */
static void redp2p_cli_pub_connect(kc_redp2p_client_t *client, void *userdata)
{
    redp2p_cli_pub_state_t *owner = (redp2p_cli_pub_state_t *)userdata;
    redp2p_cli_pub_client_t *state;
    redp2p_cli_fd_t fd;

    if (!owner || !client || atomic_load(&owner->stopping)) {
        kc_redp2p_client_close(client);
        return;
    }
    fd = redp2p_cli_connect_local(owner->protocol, owner->port);
    if (REDP2P_CLI_ISERR(fd)) {
        kc_redp2p_client_close(client);
        return;
    }
    state = (redp2p_cli_pub_client_t *)calloc(1, sizeof(*state));
    if (!state) {
        REDP2P_CLI_FD_CLOSE(fd);
        kc_redp2p_client_close(client);
        return;
    }
    state->owner = owner;
    state->client = client;
    state->fd = fd;

    redp2p_cli_mutex_lock(&owner->mutex);
    if (atomic_load(&owner->stopping)) {
        redp2p_cli_mutex_unlock(&owner->mutex);
        REDP2P_CLI_FD_CLOSE(fd);
        free(state);
        kc_redp2p_client_close(client);
        return;
    }
    state->next = owner->clients;
    owner->clients = state;
    redp2p_cli_mutex_unlock(&owner->mutex);

    if (redp2p_cli_thread_start(&state->thread,
        redp2p_cli_pub_backend_worker, state) != 0)
    {
        redp2p_cli_mutex_lock(&owner->mutex);
        if (!REDP2P_CLI_ISERR(state->fd)) {
            REDP2P_CLI_FD_CLOSE(state->fd);
            state->fd = REDP2P_CLI_FD_INVALID;
        }
        redp2p_cli_mutex_unlock(&owner->mutex);
        kc_redp2p_client_close(client);
        return;
    }
    state->thread_started = 1;
}

/**
 * Forwards peer bytes from the publisher channel to its local backend socket.
 * @param input Publisher receive event.
 * @param userdata Publisher CLI state.
 * @return None.
 */
static void redp2p_cli_pub_receive(const kc_redp2p_pub_input_t *input,
    void *userdata)
{
    redp2p_cli_pub_state_t *owner = (redp2p_cli_pub_state_t *)userdata;
    redp2p_cli_pub_client_t *state;
    int failed = 0;

    if (!owner || !input || !input->client ||
        atomic_load(&owner->stopping))
        return;
    redp2p_cli_mutex_lock(&owner->mutex);
    for (state = owner->clients; state; state = state->next) {
        if (state->client != input->client) continue;
        if (!REDP2P_CLI_ISERR(state->fd) &&
            redp2p_cli_socket_send(state->fd, owner->protocol,
                input->data, input->size) != 0)
            failed = 1;
        break;
    }
    redp2p_cli_mutex_unlock(&owner->mutex);
    if (failed) kc_redp2p_client_close(input->client);
}

/**
 * Stops all active CLI publisher forwarding workers.
 * @param state Publisher CLI state.
 * @return None.
 */
static void redp2p_cli_pub_state_stop(redp2p_cli_pub_state_t *state)
{
    redp2p_cli_pub_client_t *client;

    atomic_store(&state->stopping, 1);
    redp2p_cli_mutex_lock(&state->mutex);
    for (client = state->clients; client; client = client->next)
        redp2p_cli_socket_shutdown(client->fd);
    redp2p_cli_mutex_unlock(&state->mutex);
    for (client = state->clients; client; client = client->next) {
        if (client->thread_started)
            redp2p_cli_thread_join(client->thread);
    }
}

/**
 * Releases all CLI publisher forwarding resources.
 * @param state Publisher CLI state.
 * @return None.
 */
static void redp2p_cli_pub_state_destroy(redp2p_cli_pub_state_t *state)
{
    redp2p_cli_pub_client_t *client = state->clients;

    while (client) {
        redp2p_cli_pub_client_t *next = client->next;
        if (!REDP2P_CLI_ISERR(client->fd)) REDP2P_CLI_FD_CLOSE(client->fd);
        free(client);
        client = next;
    }
    redp2p_cli_mutex_destroy(&state->mutex);
}

/**
 * Forwards peer TCP bytes to one local CLI client socket.
 * @param data Peer bytes.
 * @param size Byte count.
 * @param userdata TCP forwarding session.
 * @return None.
 */
static void redp2p_cli_con_tcp_receive(const void *data, size_t size,
    void *userdata)
{
    redp2p_cli_con_tcp_session_t *session =
        (redp2p_cli_con_tcp_session_t *)userdata;

    if (!session || atomic_load(&session->owner->stopping)) return;
    (void)redp2p_cli_socket_send(session->fd, KC_REDP2P_TCP, data, size);
}

/**
 * Runs one local TCP client over one REDP2P peer channel.
 * @param arg TCP forwarding session.
 * @return Platform thread result.
 */
REDP2P_CLI_THREAD(redp2p_cli_con_tcp_worker)
{
    redp2p_cli_con_tcp_session_t *session =
        (redp2p_cli_con_tcp_session_t *)arg;
    kc_redp2p_con_options_t options;
    unsigned char buffer[REDP2P_CLI_BUF];
    int status;

    memset(&options, 0, sizeof(options));
    options.id = session->owner->id;
    options.index = session->owner->index;
    options.stun = session->owner->stun;
    options.turn = session->owner->turn;
    options.turn_user = session->owner->turn_user;
    options.turn_pass = session->owner->turn_pass;
    options.receive = redp2p_cli_con_tcp_receive;
    options.userdata = session;
    status = kc_redp2p_con(&session->con, &options);
    if (status == KC_REDP2P_OK) {
        while (!atomic_load(&session->owner->stopping)) {
            int n = redp2p_cli_sock_read(session->fd, (char *)buffer,
                (int)sizeof(buffer));
            if (n <= 0) break;
            if (kc_redp2p_con_send(session->con, buffer, (size_t)n) !=
                KC_REDP2P_OK)
                break;
        }
        kc_redp2p_con_close(session->con);
        session->con = NULL;
    }
    redp2p_cli_mutex_lock(&session->owner->mutex);
    if (!REDP2P_CLI_ISERR(session->fd)) {
        REDP2P_CLI_FD_CLOSE(session->fd);
        session->fd = REDP2P_CLI_FD_INVALID;
    }
    redp2p_cli_mutex_unlock(&session->owner->mutex);
    REDP2P_CLI_THREAD_RETURN();
}

/**
 * Runs the CLI TCP loopback forwarding application.
 * @param id Target publisher identifier.
 * @param index Index endpoint.
 * @param port Local loopback port.
 * @param stun Optional STUN URL.
 * @param turn Optional TURN URL.
 * @param turn_user Optional TURN username.
 * @param turn_pass Optional TURN password.
 * @return 0 on clean shutdown, 1 on startup failure.
 */
static int redp2p_cli_con_tcp_run(const char *id, const char *index,
    uint16_t port, const char *stun, const char *turn,
    const char *turn_user, const char *turn_pass)
{
    redp2p_cli_con_tcp_state_t state;
    redp2p_cli_fd_t listener;
    int result = 0;

    memset(&state, 0, sizeof(state));
    state.id = id;
    state.index = index;
    state.stun = stun;
    state.turn = turn;
    state.turn_user = turn_user;
    state.turn_pass = turn_pass;
    if (redp2p_cli_mutex_init(&state.mutex) != 0) return 1;
    listener = redp2p_cli_bind_local(KC_REDP2P_TCP, port);
    if (REDP2P_CLI_ISERR(listener)) {
        redp2p_cli_mutex_destroy(&state.mutex);
        return 1;
    }

    while (!redp2p_cli_stop) {
        redp2p_cli_pollfd_t pollfd;
        int selected;

        memset(&pollfd, 0, sizeof(pollfd));
        pollfd.fd = listener;
        pollfd.events = REDP2P_CLI_POLLIN;
        selected = redp2p_cli_poll_wait(&pollfd, 1, 250);
        if (selected <= 0 || !redp2p_cli_poll_readable(&pollfd)) continue;
        {
            redp2p_cli_fd_t fd = accept(listener, NULL, NULL);
            redp2p_cli_con_tcp_session_t *session;
            if (REDP2P_CLI_ISERR(fd)) continue;
            session = (redp2p_cli_con_tcp_session_t *)calloc(1,
                sizeof(*session));
            if (!session) {
                REDP2P_CLI_FD_CLOSE(fd);
                continue;
            }
            session->owner = &state;
            session->fd = fd;
            redp2p_cli_mutex_lock(&state.mutex);
            session->next = state.sessions;
            state.sessions = session;
            redp2p_cli_mutex_unlock(&state.mutex);
            if (redp2p_cli_thread_start(&session->thread,
                redp2p_cli_con_tcp_worker, session) != 0)
            {
                REDP2P_CLI_FD_CLOSE(fd);
                session->fd = REDP2P_CLI_FD_INVALID;
                continue;
            }
            session->thread_started = 1;
        }
    }

    atomic_store(&state.stopping, 1);
    REDP2P_CLI_FD_CLOSE(listener);
    redp2p_cli_mutex_lock(&state.mutex);
    for (redp2p_cli_con_tcp_session_t *session = state.sessions;
        session; session = session->next)
        redp2p_cli_socket_shutdown(session->fd);
    redp2p_cli_mutex_unlock(&state.mutex);

    for (redp2p_cli_con_tcp_session_t *session = state.sessions;
        session; session = session->next)
    {
        if (session->thread_started)
            redp2p_cli_thread_join(session->thread);
    }
    while (state.sessions) {
        redp2p_cli_con_tcp_session_t *next = state.sessions->next;
        if (!REDP2P_CLI_ISERR(state.sessions->fd))
            REDP2P_CLI_FD_CLOSE(state.sessions->fd);
        free(state.sessions);
        state.sessions = next;
    }
    redp2p_cli_mutex_destroy(&state.mutex);
    return result;
}

/**
 * Forwards one peer UDP datagram to its local CLI endpoint.
 * @param data Peer datagram bytes.
 * @param size Byte count.
 * @param userdata UDP forwarding session.
 * @return None.
 */
static void redp2p_cli_con_udp_receive(const void *data, size_t size,
    void *userdata)
{
    redp2p_cli_con_udp_session_t *session =
        (redp2p_cli_con_udp_session_t *)userdata;

    if (!session || atomic_load(&session->owner->stopping)) return;
    (void)sendto(session->owner->fd, (const char *)data, (int)size, 0,
        (const struct sockaddr *)&session->address, session->address_len);
}

/**
 * Finds the peer channel associated with one local UDP endpoint.
 * @param state UDP forwarding state.
 * @param address Local endpoint address.
 * @return Matching session, or NULL when absent.
 */
static redp2p_cli_con_udp_session_t *redp2p_cli_con_udp_find(
    redp2p_cli_con_udp_state_t *state,
    const struct sockaddr_storage *address)
{
    redp2p_cli_con_udp_session_t *session;

    for (session = state->sessions; session; session = session->next) {
        if (redp2p_cli_sockaddr_equal(&session->address, address))
            return session;
    }
    return NULL;
}

/**
 * Opens one REDP2P channel for a local UDP endpoint.
 * @param state UDP forwarding state.
 * @param session Session receiving the channel.
 * @return KC_REDP2P_OK on success or a public error code.
 */
static int redp2p_cli_con_udp_open(redp2p_cli_con_udp_state_t *state,
    redp2p_cli_con_udp_session_t *session)
{
    kc_redp2p_con_options_t options;

    memset(&options, 0, sizeof(options));
    options.id = state->id;
    options.index = state->index;
    options.stun = state->stun;
    options.turn = state->turn;
    options.turn_user = state->turn_user;
    options.turn_pass = state->turn_pass;
    options.receive = redp2p_cli_con_udp_receive;
    options.userdata = session;
    return kc_redp2p_con(&session->con, &options);
}

/**
 * Runs the CLI UDP loopback forwarding application.
 * @param id Target publisher identifier.
 * @param index Index endpoint.
 * @param port Local loopback port.
 * @param stun Optional STUN URL.
 * @param turn Optional TURN URL.
 * @param turn_user Optional TURN username.
 * @param turn_pass Optional TURN password.
 * @return 0 on clean shutdown, 1 on startup failure.
 */
static int redp2p_cli_con_udp_run(const char *id, const char *index,
    uint16_t port, const char *stun, const char *turn,
    const char *turn_user, const char *turn_pass)
{
    redp2p_cli_con_udp_state_t state;
    unsigned char buffer[REDP2P_CLI_BUF];

    memset(&state, 0, sizeof(state));
    state.id = id;
    state.index = index;
    state.stun = stun;
    state.turn = turn;
    state.turn_user = turn_user;
    state.turn_pass = turn_pass;
    state.fd = redp2p_cli_bind_local(KC_REDP2P_UDP, port);
    if (REDP2P_CLI_ISERR(state.fd)) return 1;

    while (!redp2p_cli_stop) {
        redp2p_cli_pollfd_t pollfd;
        struct sockaddr_storage from;
        socklen_t from_len;
        int selected;
        int n;

        memset(&pollfd, 0, sizeof(pollfd));
        pollfd.fd = state.fd;
        pollfd.events = REDP2P_CLI_POLLIN;
        selected = redp2p_cli_poll_wait(&pollfd, 1, 250);
        if (selected <= 0 || !redp2p_cli_poll_readable(&pollfd)) continue;
        from_len = sizeof(from);
        n = (int)recvfrom(state.fd, (char *)buffer, sizeof(buffer), 0,
            (struct sockaddr *)&from, &from_len);
        if (n < 0) continue;
        {
            redp2p_cli_con_udp_session_t *session =
                redp2p_cli_con_udp_find(&state, &from);
            if (!session) {
                session = (redp2p_cli_con_udp_session_t *)calloc(1,
                    sizeof(*session));
                if (!session) continue;
                session->owner = &state;
                session->address = from;
                session->address_len = from_len;
                if (redp2p_cli_con_udp_open(&state, session) != KC_REDP2P_OK) {
                    free(session);
                    continue;
                }
                session->next = state.sessions;
                state.sessions = session;
            } else if (!session->con) {
                if (redp2p_cli_con_udp_open(&state, session) != KC_REDP2P_OK)
                    continue;
            }
            if (kc_redp2p_con_send(session->con, buffer, (size_t)n) !=
                KC_REDP2P_OK)
            {
                kc_redp2p_con_close(session->con);
                session->con = NULL;
            }
        }
    }

    atomic_store(&state.stopping, 1);
    while (state.sessions) {
        redp2p_cli_con_udp_session_t *next = state.sessions->next;
        kc_redp2p_con_close(state.sessions->con);
        free(state.sessions);
        state.sessions = next;
    }
    REDP2P_CLI_FD_CLOSE(state.fd);
    return 0;
}

/**
 * Prints command usage information.
 * @param name Program executable name.
 * @return None.
 */
static void redp2p_cli_usage(const char *name)
{
    printf("Usage: %s <command> [options]\n", name);
    printf("\n");
    printf("Commands:\n");
    printf("  idx <port> [--seats <N>] [--pow <N>] [--max-consumers <N>]\n");
    printf("  pub <id>@<index[:port]> --tcp <port> [peer options]\n");
    printf("  pub <id>@<index[:port]> --udp <port> [peer options]\n");
    printf("  con <id>@<index[:port]> <local-port> [peer options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help      Show this help\n");
    printf("  -v, --version   Show version\n");
    printf("  --stun <url>    Optional STUN URL\n");
    printf("  --turn <url>    Optional TURN URL\n");
    printf("  --turn-user <u> Optional TURN username\n");
    printf("  --turn-pass <p> Optional TURN password\n");
    printf("\n");
    printf("Environment:\n");
    printf("  REDP2P_SEATS          Index publisher capacity; unset means unlimited\n");
    printf("  REDP2P_POW            Index registration PoW bits (0..32)\n");
    printf("  REDP2P_PASS           Index/pub registration password\n");
    printf("  REDP2P_VIP            Reserved index IDs as '<id> <pass> ...'\n");
    printf("  REDP2P_MAX_CONSUMERS_PER_PUBLISHER\n");
    printf("  REDP2P_STUN           Optional STUN URL for pub/con\n");
    printf("  REDP2P_TURN           Optional TURN URL for pub/con\n");
    printf("  REDP2P_TURN_USER      Optional TURN username for pub/con\n");
    printf("  REDP2P_TURN_PASS      Optional TURN password for pub/con\n");
}

/**
 * Parses the REDP2P_VIP environment value into structured entries.
 * @param text VIP token text.
 * @param out Destination VIP array.
 * @param out_count Destination VIP count.
 * @param out_storage Destination mutable backing storage.
 * @return 1 on success, 0 on invalid input or allocation failure.
 */
static int redp2p_cli_vips(const char *text, kc_redp2p_vip_t **out,
    size_t *out_count, char **out_storage)
{
    char *copy;
    char *p;
    size_t tokens;
    size_t count;
    kc_redp2p_vip_t *vips;

    *out = NULL;
    *out_count = 0;
    *out_storage = NULL;
    if (!text || !text[0]) return 1;

    copy = (char *)malloc(strlen(text) + 1);
    if (!copy) return 0;
    memcpy(copy, text, strlen(text) + 1);

    tokens = 0;
    p = copy;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (!*p) break;
        tokens++;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
            p++;
    }
    if (tokens == 0 || (tokens & 1u)) {
        free(copy);
        return 0;
    }
    count = tokens / 2;
    vips = (kc_redp2p_vip_t *)calloc(count, sizeof(*vips));
    if (!vips) {
        free(copy);
        return 0;
    }

    p = copy;
    for (size_t i = 0; i < count; i++) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        vips[i].id = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
            p++;
        if (*p) *p++ = '\0';
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        vips[i].pass = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
            p++;
        if (*p) *p++ = '\0';
    }
    *out = vips;
    *out_count = count;
    *out_storage = copy;
    return 1;
}

/**
 * Runs the idx command.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
static int redp2p_cli_idx(int argc, char **argv)
{
    kc_redp2p_idx_options_t options;
    kc_redp2p_idx_t *idx = NULL;
    kc_redp2p_vip_t *vips = NULL;
    char *vip_storage = NULL;
    size_t vip_storage_len = 0;
    size_t seats_value = 0;
    const size_t *seats = NULL;
    size_t max_consumers = 0;
    uint16_t port;
    unsigned int pow = 0;
    const char *value;
    int status;

    if (argc < 3 || !redp2p_cli_u16(argv[2], &port)) {
        fprintf(stderr, "redp2p: idx requires a port\n");
        return 1;
    }

    value = getenv("REDP2P_SEATS");
    if (value) {
        if (!redp2p_cli_size(value, &seats_value)) {
            fprintf(stderr, "redp2p: invalid REDP2P_SEATS\n");
            return 1;
        }
        seats = &seats_value;
    }
    value = getenv("REDP2P_POW");
    if (value && !redp2p_cli_uint(value, 32, &pow)) {
        fprintf(stderr, "redp2p: invalid REDP2P_POW\n");
        return 1;
    }
    value = getenv("REDP2P_MAX_CONSUMERS_PER_PUBLISHER");
    if (value && !redp2p_cli_size(value, &max_consumers)) {
        fprintf(stderr, "redp2p: invalid REDP2P_MAX_CONSUMERS_PER_PUBLISHER\n");
        return 1;
    }

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--seats") == 0) {
            if (++i >= argc || !redp2p_cli_size(argv[i], &seats_value)) {
                fprintf(stderr, "redp2p: --seats requires a nonnegative integer\n");
                return 1;
            }
            seats = &seats_value;
        } else if (strcmp(argv[i], "--pow") == 0) {
            if (++i >= argc || !redp2p_cli_uint(argv[i], 32, &pow)) {
                fprintf(stderr, "redp2p: --pow requires 0..32\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--max-consumers") == 0) {
            if (++i >= argc || !redp2p_cli_size(argv[i], &max_consumers)) {
                fprintf(stderr, "redp2p: --max-consumers requires a nonnegative integer\n");
                return 1;
            }
        } else {
            fprintf(stderr, "redp2p: unknown idx option '%s'\n", argv[i]);
            return 1;
        }
    }
    memset(&options, 0, sizeof(options));
    options.port = port;
    options.seats = seats;
    options.pow = pow;
    options.pass = getenv("REDP2P_PASS");
    options.max_consumers = max_consumers;
    value = getenv("REDP2P_VIP");
    if (value) vip_storage_len = strlen(value) + 1;
    if (!redp2p_cli_vips(value, &vips,
        &options.vip_count, &vip_storage))
    {
        fprintf(stderr, "redp2p: invalid REDP2P_VIP\n");
        return 1;
    }
    options.vips = vips;

    status = kc_redp2p_idx(&idx, &options);
    free(vips);
    if (vip_storage) {
        memset(vip_storage, 0, vip_storage_len);
        free(vip_storage);
    }
    if (status != KC_REDP2P_OK) {
        fprintf(stderr, "redp2p: idx failed: %s\n",
            kc_redp2p_strerror(status));
        return 1;
    }

    fprintf(stderr, "redp2p: idx listening on port %u\n", (unsigned)port);
    redp2p_cli_stop = 0;
    signal(SIGINT, redp2p_cli_signal);
    signal(SIGTERM, redp2p_cli_signal);
    while (!redp2p_cli_stop) redp2p_cli_sleep();
    kc_redp2p_idx_close(idx);
    return 0;
}

/**
 * Runs the pub command.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
static int redp2p_cli_pub(int argc, char **argv)
{
    kc_redp2p_pub_options_t options;
    kc_redp2p_pub_t *pub = NULL;
    redp2p_cli_pub_state_t state;
    char id[KC_REDP2P_ID_MAX + 1];
    char index[320];
    uint16_t port = 0;
    int protocol = 0;
    const char *stun = getenv("REDP2P_STUN");
    const char *turn = getenv("REDP2P_TURN");
    const char *turn_user = getenv("REDP2P_TURN_USER");
    const char *turn_pass = getenv("REDP2P_TURN_PASS");
    int status;

    if (argc < 5 || !redp2p_cli_spec(argv[2], id, index)) {
        fprintf(stderr,
            "redp2p: usage: %s pub <id>@<index[:port]> --tcp|--udp <port>\n",
            argv[0]);
        return 1;
    }
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--tcp") == 0 || strcmp(argv[i], "--udp") == 0) {
            if (protocol || ++i >= argc || !redp2p_cli_u16(argv[i], &port)) {
                fprintf(stderr, "redp2p: choose one protocol and valid port\n");
                return 1;
            }
            protocol = strcmp(argv[i - 1], "--tcp") == 0 ?
                KC_REDP2P_TCP : KC_REDP2P_UDP;
        } else if (strcmp(argv[i], "--stun") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --stun requires a URL\n");
                return 1;
            }
            stun = argv[i];
        } else if (strcmp(argv[i], "--turn") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --turn requires a URL\n");
                return 1;
            }
            turn = argv[i];
        } else if (strcmp(argv[i], "--turn-user") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --turn-user requires a value\n");
                return 1;
            }
            turn_user = argv[i];
        } else if (strcmp(argv[i], "--turn-pass") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --turn-pass requires a value\n");
                return 1;
            }
            turn_pass = argv[i];
        } else {
            fprintf(stderr, "redp2p: unknown pub option '%s'\n", argv[i]);
            return 1;
        }
    }
    if (!protocol || !port) {
        fprintf(stderr, "redp2p: pub requires --tcp <port> or --udp <port>\n");
        return 1;
    }
    if (!turn && (turn_user || turn_pass)) {
        fprintf(stderr, "redp2p: TURN credentials require --turn or REDP2P_TURN\n");
        return 1;
    }
    if (redp2p_cli_platform_init() != 0) {
        fprintf(stderr, "redp2p: pub failed: network failure\n");
        return 1;
    }
    memset(&state, 0, sizeof(state));
    state.protocol = protocol;
    state.port = port;
    if (redp2p_cli_mutex_init(&state.mutex) != 0) {
        redp2p_cli_platform_cleanup();
        return 1;
    }

    memset(&options, 0, sizeof(options));
    options.id = id;
    options.index = index;
    options.protocol = protocol;
    options.pass = getenv("REDP2P_PASS");
    options.stun = stun;
    options.turn = turn;
    options.turn_user = turn_user;
    options.turn_pass = turn_pass;
    options.connect = redp2p_cli_pub_connect;
    options.receive = redp2p_cli_pub_receive;
    options.userdata = &state;

    status = kc_redp2p_pub(&pub, &options);
    if (status != KC_REDP2P_OK) {
        fprintf(stderr, "redp2p: pub failed: %s\n",
            kc_redp2p_strerror(status));
        redp2p_cli_pub_state_destroy(&state);
        redp2p_cli_platform_cleanup();
        return 1;
    }

    fprintf(stderr, "redp2p: '%s' published from local port %u\n",
        id, (unsigned)port);
    redp2p_cli_stop = 0;
    signal(SIGINT, redp2p_cli_signal);
    signal(SIGTERM, redp2p_cli_signal);
    while (!redp2p_cli_stop) redp2p_cli_sleep();

    redp2p_cli_pub_state_stop(&state);
    kc_redp2p_pub_close(pub);
    redp2p_cli_pub_state_destroy(&state);
    redp2p_cli_platform_cleanup();
    return 0;
}

/**
 * Arguments for one consumer forwarding runner.
 */
typedef struct {
    const char *id;
    const char *index;
    const char *stun;
    const char *turn;
    const char *turn_user;
    const char *turn_pass;
    uint16_t port;
    int protocol;
    int result;
} redp2p_cli_con_runner_t;

/**
 * Runs one local consumer forwarding socket.
 * @param arg Consumer runner arguments.
 * @return Platform thread result.
 */
REDP2P_CLI_THREAD(redp2p_cli_con_runner)
{
    redp2p_cli_con_runner_t *runner = (redp2p_cli_con_runner_t *)arg;

    runner->result = runner->protocol == KC_REDP2P_TCP ?
        redp2p_cli_con_tcp_run(runner->id, runner->index, runner->port,
            runner->stun, runner->turn, runner->turn_user,
            runner->turn_pass) :
        redp2p_cli_con_udp_run(runner->id, runner->index, runner->port,
            runner->stun, runner->turn, runner->turn_user,
            runner->turn_pass);
    REDP2P_CLI_THREAD_RETURN();
}

/**
 * Runs the con command.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
static int redp2p_cli_con(int argc, char **argv)
{
    char id[KC_REDP2P_ID_MAX + 1];
    char index[320];
    uint16_t port;
    const char *stun = getenv("REDP2P_STUN");
    const char *turn = getenv("REDP2P_TURN");
    const char *turn_user = getenv("REDP2P_TURN_USER");
    const char *turn_pass = getenv("REDP2P_TURN_PASS");
    redp2p_cli_con_runner_t tcp;
    redp2p_cli_con_runner_t udp;
    redp2p_cli_thread_t tcp_thread;
    redp2p_cli_thread_t udp_thread;
    int tcp_started = 0;
    int udp_started = 0;

    if (argc < 4 || !redp2p_cli_spec(argv[2], id, index) ||
        !redp2p_cli_u16(argv[3], &port))
    {
        fprintf(stderr,
            "redp2p: usage: %s con <id>@<index[:port]> <local-port> [peer options]\n",
            argv[0]);
        return 1;
    }
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--stun") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --stun requires a URL\n");
                return 1;
            }
            stun = argv[i];
        } else if (strcmp(argv[i], "--turn") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --turn requires a URL\n");
                return 1;
            }
            turn = argv[i];
        } else if (strcmp(argv[i], "--turn-user") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --turn-user requires a value\n");
                return 1;
            }
            turn_user = argv[i];
        } else if (strcmp(argv[i], "--turn-pass") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "redp2p: --turn-pass requires a value\n");
                return 1;
            }
            turn_pass = argv[i];
        } else {
            fprintf(stderr, "redp2p: unknown con option '%s'\n", argv[i]);
            return 1;
        }
    }
    if (!turn && (turn_user || turn_pass)) {
        fprintf(stderr, "redp2p: TURN credentials require --turn or REDP2P_TURN\n");
        return 1;
    }
    if (redp2p_cli_platform_init() != 0) {
        fprintf(stderr, "redp2p: con failed: network failure\n");
        return 1;
    }

    memset(&tcp, 0, sizeof(tcp));
    memset(&udp, 0, sizeof(udp));
    tcp.id = udp.id = id;
    tcp.index = udp.index = index;
    tcp.stun = udp.stun = stun;
    tcp.turn = udp.turn = turn;
    tcp.turn_user = udp.turn_user = turn_user;
    tcp.turn_pass = udp.turn_pass = turn_pass;
    tcp.port = udp.port = port;
    tcp.protocol = KC_REDP2P_TCP;
    udp.protocol = KC_REDP2P_UDP;
    tcp.result = udp.result = 1;

    redp2p_cli_stop = 0;
    signal(SIGINT, redp2p_cli_signal);
    signal(SIGTERM, redp2p_cli_signal);
    if (redp2p_cli_thread_start(&tcp_thread, redp2p_cli_con_runner, &tcp) == 0)
        tcp_started = 1;
    if (redp2p_cli_thread_start(&udp_thread, redp2p_cli_con_runner, &udp) == 0)
        udp_started = 1;
    if (!tcp_started && !udp_started) {
        redp2p_cli_platform_cleanup();
        fprintf(stderr, "redp2p: con failed: network failure\n");
        return 1;
    }

    fprintf(stderr, "redp2p: tunnel to '%s' available on 127.0.0.1:%u\n",
        id, (unsigned)port);
    if (tcp_started) redp2p_cli_thread_join(tcp_thread);
    if (udp_started) redp2p_cli_thread_join(udp_thread);
    redp2p_cli_platform_cleanup();
    return tcp.result != 0 && udp.result != 0 ? 1 : 0;
}

/**
 * Runs the REDP2P command-line interface.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0)
    {
        redp2p_cli_usage(argv[0]);
        return argc < 2 ? 1 : 0;
    }
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("%llu\n", (unsigned long long)kc_redp2p_version());
        return 0;
    }
    if (strcmp(argv[1], "idx") == 0) return redp2p_cli_idx(argc, argv);
    if (strcmp(argv[1], "pub") == 0) return redp2p_cli_pub(argc, argv);
    if (strcmp(argv[1], "con") == 0) return redp2p_cli_con(argc, argv);

    fprintf(stderr, "redp2p: unknown command '%s'\n", argv[1]);
    return 1;
}
