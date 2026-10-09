#define _POSIX_C_SOURCE 200809L
/**
 * webp2p.c - REDP2P WebRTC command-line peer.
 * Summary: Publishes and consumes local TCP services through REDP2P WebRTC.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "protocol.h"
#include "rtc.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <limits.h>
#include <unistd.h>
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
typedef SSIZE_T ssize_t;
static inline int webp2p_socket_close(int fd)
{
    return closesocket((SOCKET)fd);
}
static inline int webp2p_socket_send(int fd, const void *data, size_t size,
    int flags)
{
    int count = size > INT_MAX ? INT_MAX : (int)size;
    return send((SOCKET)fd, (const char *)data, count, flags);
}
static inline int webp2p_socket_recv(int fd, void *data, size_t size,
    int flags)
{
    int count = size > INT_MAX ? INT_MAX : (int)size;
    return recv((SOCKET)fd, (char *)data, count, flags);
}
static inline int webp2p_socket_setsockopt(int fd, int level, int option,
    const void *value, size_t size)
{
    return setsockopt((SOCKET)fd, level, option, (const char *)value,
        (int)size);
}
#define close(fd) webp2p_socket_close(fd)
#define send(fd, data, size, flags) \
    webp2p_socket_send((fd), (data), (size), (flags))
#define recv(fd, data, size, flags) \
    webp2p_socket_recv((fd), (data), (size), (flags))
#define setsockopt(fd, level, option, value, size) \
    webp2p_socket_setsockopt((fd), (level), (option), (value), (size))
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WEBP2P_BUILD_VERSION
#define WEBP2P_BUILD_VERSION "0"
#endif

#define WEBP2P_BUFFER_SIZE 16384
#define WEBP2P_ICE_TIMEOUT_MS 15000
#define WEBP2P_CONNECT_TIMEOUT_MS 30000
#define WEBP2P_HEARTBEAT_MS 15000
#define WEBP2P_PUBLISHER_POLL_MS 3000
#define WEBP2P_CONSUMER_POLL_MS 500

typedef struct webp2p_session webp2p_session_t;

typedef struct {
    const char *id;
    const char *index;
    const char *pass;
    const char *stun;
    const char *turn;
    const char *turn_user;
    const char *turn_pass;
    uint16_t port;
    int force_turn;
    int ordered;
} webp2p_options_t;

struct webp2p_session {
    int fd;
    webp2p_rtc_peer_t *peer;
    pthread_t thread;
    int thread_started;
    _Atomic int done;
    _Atomic int failed;
    _Atomic int peer_closed;
    char connection[33];
    const webp2p_options_t *options;
    webp2p_session_t *next;
};

static volatile sig_atomic_t webp2p_stop;

static void webp2p_signal(int signal_number)
{
    (void)signal_number;
    webp2p_stop = 1;
}

static void webp2p_sleep(unsigned milliseconds)
{
    struct timespec delay;

    delay.tv_sec = (time_t)(milliseconds / 1000u);
    delay.tv_nsec = (long)(milliseconds % 1000u) * 1000000L;
    (void)nanosleep(&delay, NULL);
}

static int webp2p_write_all(int fd, const void *data, size_t size)
{
    const unsigned char *cursor = (const unsigned char *)data;

    while (size > 0) {
        ssize_t written = send(fd, cursor, size, MSG_NOSIGNAL);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return -1;
        cursor += (size_t)written;
        size -= (size_t)written;
    }
    return 0;
}

static int webp2p_connect_local(uint16_t port)
{
    struct sockaddr_in address;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(0x7f000001u);
    if (connect(fd, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int webp2p_listen(uint16_t port)
{
    struct sockaddr_in address;
    int fd;
    int reuse = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (const struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(fd, 64) != 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

static void webp2p_session_receive(const void *data, size_t size,
    void *userdata)
{
    webp2p_session_t *session = (webp2p_session_t *)userdata;

    if (!session || session->fd < 0 || atomic_load(&session->done)) return;
    if (webp2p_write_all(session->fd, data, size) != 0) {
        atomic_store(&session->failed, 1);
        (void)shutdown(session->fd, SHUT_RDWR);
    }
}

static void webp2p_session_closed(void *userdata)
{
    webp2p_session_t *session = (webp2p_session_t *)userdata;

    if (!session) return;
    atomic_store(&session->peer_closed, 1);
    if (session->fd >= 0) (void)shutdown(session->fd, SHUT_RDWR);
}

static void *webp2p_session_pump(void *userdata)
{
    webp2p_session_t *session = (webp2p_session_t *)userdata;
    unsigned char buffer[WEBP2P_BUFFER_SIZE];

    if (webp2p_rtc_wait_open(session->peer, WEBP2P_CONNECT_TIMEOUT_MS) != 0) {
        atomic_store(&session->failed, 1);
        goto done;
    }
    while (!webp2p_stop && !atomic_load(&session->peer_closed) &&
        !atomic_load(&session->failed))
    {
        ssize_t received = recv(session->fd, buffer, sizeof(buffer), 0);
        if (received < 0 && errno == EINTR) continue;
        if (received <= 0) break;
        if (webp2p_rtc_send(session->peer, buffer, (size_t)received) != 0) {
            atomic_store(&session->failed, 1);
            break;
        }
    }
done:
    if (session->fd >= 0) {
        (void)shutdown(session->fd, SHUT_RDWR);
        close(session->fd);
        session->fd = -1;
    }
    webp2p_rtc_close(session->peer);
    session->peer = NULL;
    atomic_store(&session->done, 1);
    return NULL;
}

static int webp2p_session_active(webp2p_session_t *sessions,
    const char *connection)
{
    webp2p_session_t *session;

    for (session = sessions; session; session = session->next)
        if (strcmp(session->connection, connection) == 0) return 1;
    return 0;
}

static void webp2p_sessions_reap(webp2p_session_t **sessions)
{
    webp2p_session_t **cursor = sessions;

    while (*cursor) {
        webp2p_session_t *session = *cursor;
        if (!atomic_load(&session->done)) {
            cursor = &session->next;
            continue;
        }
        *cursor = session->next;
        if (session->thread_started) pthread_join(session->thread, NULL);
        free(session);
    }
}

static void webp2p_sessions_stop(webp2p_session_t **sessions)
{
    webp2p_session_t *session;

    for (session = *sessions; session; session = session->next) {
        if (session->fd >= 0) (void)shutdown(session->fd, SHUT_RDWR);
    }
    while (*sessions) {
        session = *sessions;
        *sessions = session->next;
        if (session->thread_started) pthread_join(session->thread, NULL);
        else {
            if (session->fd >= 0) close(session->fd);
            webp2p_rtc_close(session->peer);
        }
        free(session);
    }
}

static int webp2p_publisher_accept(webp2p_publisher_t *publisher,
    const webp2p_options_t *options, const webp2p_pending_t *pending,
    webp2p_session_t **sessions)
{
    webp2p_description_t answer;
    webp2p_session_t *session;

    memset(&answer, 0, sizeof(answer));
    session = (webp2p_session_t *)calloc(1, sizeof(*session));
    if (!session) return -1;
    session->fd = -1;
    session->options = options;
    memcpy(session->connection, pending->connection, sizeof(session->connection));
    session->fd = webp2p_connect_local(options->port);
    if (session->fd < 0) {
        fprintf(stderr, "webp2p: cannot connect local service on port %u\n",
            (unsigned)options->port);
        goto fail;
    }
    session->peer = webp2p_rtc_create(0, options->stun, options->turn,
        options->turn_user, options->turn_pass, options->force_turn,
        options->ordered, webp2p_session_receive, webp2p_session_closed,
        session);
    if (!session->peer ||
        webp2p_rtc_set_remote(session->peer, &pending->offer) != 0 ||
        webp2p_rtc_make_local(session->peer, "answer", &answer,
            WEBP2P_ICE_TIMEOUT_MS) != 0 ||
        webp2p_pub_answer(publisher, options->id, pending->connection,
            &answer) != 0)
    {
        fprintf(stderr, "webp2p: failed to accept peer %s\n",
            pending->connection);
        goto fail;
    }
    webp2p_description_free(&answer);
    if (pthread_create(&session->thread, NULL, webp2p_session_pump,
        session) != 0)
        goto fail;
    session->thread_started = 1;
    session->next = *sessions;
    *sessions = session;
    return 0;
fail:
    webp2p_description_free(&answer);
    if (session->fd >= 0) close(session->fd);
    webp2p_rtc_close(session->peer);
    free(session);
    return -1;
}

static int webp2p_publisher_run(const webp2p_options_t *options)
{
    webp2p_publisher_t publisher;
    webp2p_session_t *sessions = NULL;
    uint64_t heartbeat_at;
    uint64_t poll_at;
    int result = 0;

    if (webp2p_pub_register(&publisher, options->index, options->id,
        options->pass) != 0)
    {
        fprintf(stderr, "webp2p: publisher registration failed\n");
        return 1;
    }
    heartbeat_at = webp2p_now_ms() + WEBP2P_HEARTBEAT_MS;
    poll_at = 0;
    while (!webp2p_stop) {
        uint64_t now = webp2p_now_ms();
        webp2p_sessions_reap(&sessions);
        if (now >= heartbeat_at) {
            if (webp2p_pub_heartbeat(&publisher, options->id) != 0) {
                result = 1;
                break;
            }
            heartbeat_at = now + WEBP2P_HEARTBEAT_MS;
        }
        if (now >= poll_at) {
            webp2p_pending_t pending[WEBP2P_MAX_PENDING];
            size_t count = 0;
            size_t i;

            if (webp2p_pub_poll(&publisher, options->id, pending,
                &count) != 0)
            {
                result = 1;
                break;
            }
            for (i = 0; i < count; i++) {
                if (!webp2p_session_active(sessions, pending[i].connection))
                    (void)webp2p_publisher_accept(&publisher, options,
                        &pending[i], &sessions);
                webp2p_pending_free(&pending[i]);
            }
            poll_at = now + WEBP2P_PUBLISHER_POLL_MS;
        }
        webp2p_sleep(100);
    }
    if (webp2p_pub_deregister(&publisher, options->id) != 0)
        result = 1;
    webp2p_sessions_stop(&sessions);
    return result;
}

static void *webp2p_consumer_session(void *userdata)
{
    webp2p_session_t *session = (webp2p_session_t *)userdata;
    const webp2p_options_t *options = session->options;
    webp2p_description_t offer;
    webp2p_description_t answer;
    webp2p_connection_t connection;
    unsigned char buffer[WEBP2P_BUFFER_SIZE];
    uint64_t deadline;
    int has_answer = 0;

    memset(&offer, 0, sizeof(offer));
    memset(&answer, 0, sizeof(answer));
    session->peer = webp2p_rtc_create(1, options->stun, options->turn,
        options->turn_user, options->turn_pass, options->force_turn,
        options->ordered, webp2p_session_receive, webp2p_session_closed,
        session);
    if (!session->peer || webp2p_rtc_make_local(session->peer, "offer",
        &offer, WEBP2P_ICE_TIMEOUT_MS) != 0 ||
        webp2p_con_connect(options->index, options->id, &offer,
            &connection) != 0)
    {
        atomic_store(&session->failed, 1);
        goto done;
    }
    deadline = webp2p_now_ms() + WEBP2P_CONNECT_TIMEOUT_MS;
    if (connection.expires_at <= UINT64_MAX / 1000u &&
        connection.expires_at * 1000u < deadline)
        deadline = connection.expires_at * 1000u;
    while (!webp2p_stop && webp2p_now_ms() < deadline && !has_answer) {
        if (webp2p_con_poll(options->index, &connection, &answer,
            &has_answer) != 0)
        {
            atomic_store(&session->failed, 1);
            goto done;
        }
        if (!has_answer) webp2p_sleep(WEBP2P_CONSUMER_POLL_MS);
    }
    if (!has_answer || webp2p_rtc_set_remote(session->peer, &answer) != 0 ||
        webp2p_rtc_wait_open(session->peer, WEBP2P_CONNECT_TIMEOUT_MS) != 0)
    {
        atomic_store(&session->failed, 1);
        goto done;
    }
    while (!webp2p_stop && !atomic_load(&session->peer_closed) &&
        !atomic_load(&session->failed))
    {
        ssize_t received = recv(session->fd, buffer, sizeof(buffer), 0);
        if (received < 0 && errno == EINTR) continue;
        if (received <= 0) break;
        if (webp2p_rtc_send(session->peer, buffer, (size_t)received) != 0) {
            atomic_store(&session->failed, 1);
            break;
        }
    }
done:
    webp2p_description_free(&offer);
    webp2p_description_free(&answer);
    if (session->fd >= 0) {
        (void)shutdown(session->fd, SHUT_RDWR);
        close(session->fd);
        session->fd = -1;
    }
    webp2p_rtc_close(session->peer);
    session->peer = NULL;
    atomic_store(&session->done, 1);
    return NULL;
}

static int webp2p_consumer_run(const webp2p_options_t *options)
{
    webp2p_session_t *sessions = NULL;
    int listener;

    listener = webp2p_listen(options->port);
    if (listener < 0) {
        fprintf(stderr, "webp2p: cannot listen on port %u\n",
            (unsigned)options->port);
        return 1;
    }
    while (!webp2p_stop) {
        fd_set reads;
        struct timeval timeout;
        int selected;

        webp2p_sessions_reap(&sessions);
        FD_ZERO(&reads);
        FD_SET(listener, &reads);
        timeout.tv_sec = 0;
        timeout.tv_usec = 250000;
        selected = select(listener + 1, &reads, NULL, NULL, &timeout);
        if (selected < 0 && errno == EINTR) continue;
        if (selected <= 0 || !FD_ISSET(listener, &reads)) continue;
        {
            int fd = accept(listener, NULL, NULL);
            webp2p_session_t *session;
            if (fd < 0) continue;
            session = (webp2p_session_t *)calloc(1, sizeof(*session));
            if (!session) {
                close(fd);
                continue;
            }
            session->fd = fd;
            session->options = options;
            if (pthread_create(&session->thread, NULL,
                webp2p_consumer_session, session) != 0)
            {
                close(fd);
                free(session);
                continue;
            }
            session->thread_started = 1;
            session->next = sessions;
            sessions = session;
        }
    }
    close(listener);
    webp2p_sessions_stop(&sessions);
    return 0;
}

static int webp2p_parse_port(const char *text, uint16_t *port)
{
    unsigned long value;
    char *end;

    if (!text || !text[0]) return 0;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno || *end || value == 0 || value > 65535) return 0;
    *port = (uint16_t)value;
    return 1;
}

static int webp2p_parse_bool(const char *text, int *value)
{
    if (!text || !value) return 0;
    if (strcmp(text, "0") == 0) {
        *value = 0;
        return 1;
    }
    if (strcmp(text, "1") == 0) {
        *value = 1;
        return 1;
    }
    return 0;
}

static int webp2p_parse_spec(const char *text, char id[64],
    char index[1024])
{
    const char *at;
    size_t id_size;
    size_t index_size;

    if (!text) return 0;
    at = strchr(text, '@');
    if (!at || at == text || !at[1]) return 0;
    id_size = (size_t)(at - text);
    index_size = strlen(at + 1);
    if (id_size >= 64 || index_size >= 1024) return 0;
    memcpy(id, text, id_size);
    id[id_size] = '\0';
    memcpy(index, at + 1, index_size + 1);
    if (webp2p_validate_id(id) != 0) return 0;
    if (strncmp(index, "http://", 7) != 0 &&
        strncmp(index, "https://", 8) != 0)
        return 0;
    return 1;
}

static void webp2p_usage(const char *name)
{
    printf("Usage: %s <command> [options]\n\n", name);
    printf("Commands:\n");
    printf("  pub <id>@<index-url> -p <port>    Publish a local TCP service\n");
    printf("  con <id>@<index-url> -p <port>    Expose a published TCP service\n\n");
    printf("Publisher options:\n");
    printf("  -p, --port <port>      Local service port\n");
    printf("  -t, --transport tcp    Local transport (currently tcp)\n");
    printf("  --pass <pass>          Index registration password\n");
    printf("  --stun <url>           Optional STUN URL\n");
    printf("  --turn <url>           Optional TURN URL\n");
    printf("  --turn-user <user>     Optional TURN username\n");
    printf("  --turn-pass <pass>     Optional TURN password\n");
    printf("  --force-turn <0|1>     Disable or require TURN relay\n\n");
    printf("Consumer options:\n");
    printf("  -p, --port <port>      Local exposed port\n");
    printf("  -t, --transport tcp    Local transport (currently tcp)\n");
    printf("  --stun <url>           Optional STUN URL\n");
    printf("  --turn <url>           Optional TURN URL\n");
    printf("  --turn-user <user>     Optional TURN username\n");
    printf("  --turn-pass <pass>     Optional TURN password\n");
    printf("  --force-turn <0|1>     Disable or require TURN relay\n\n");
    printf("Common options:\n");
    printf("  -h, --help             Show this help\n");
    printf("  -v, --version          Show version\n");
}

int main(int argc, char **argv)
{
    webp2p_options_t options;
    const char *command;
    const char *spec = NULL;
    const char *transport = "tcp";
    const char *force_turn_env;
    char id[64];
    char index[1024];
    int is_publisher;
    int i;
    int result;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0))
    {
        webp2p_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0))
    {
        printf("webp2p %s\n", WEBP2P_BUILD_VERSION);
        return 0;
    }
    if (argc < 3) {
        webp2p_usage(argv[0]);
        return 2;
    }
    command = argv[1];
    is_publisher = strcmp(command, "pub") == 0;
    if (!is_publisher && strcmp(command, "con") != 0) {
        fprintf(stderr, "webp2p: unknown command: %s\n", command);
        return 2;
    }
    memset(&options, 0, sizeof(options));
    options.pass = is_publisher ? getenv("REDP2P_PASS") : NULL;
    options.stun = getenv("REDP2P_STUN");
    options.turn = getenv("REDP2P_TURN");
    options.turn_user = getenv("REDP2P_TURN_USER");
    options.turn_pass = getenv("REDP2P_TURN_PASS");
    options.ordered = 1;
    force_turn_env = getenv("REDP2P_FORCE_TURN");
    if (force_turn_env && !webp2p_parse_bool(force_turn_env,
        &options.force_turn))
    {
        fprintf(stderr, "webp2p: REDP2P_FORCE_TURN requires 0 or 1\n");
        return 2;
    }

    for (i = 2; i < argc; i++) {
        const char *arg = argv[i];
        if (arg[0] != '-') {
            if (spec) {
                fprintf(stderr, "webp2p: unexpected argument: %s\n", arg);
                return 2;
            }
            spec = arg;
        } else if (strcmp(arg, "-p") == 0 || strcmp(arg, "--port") == 0) {
            if (++i >= argc || !webp2p_parse_port(argv[i], &options.port)) {
                fprintf(stderr, "webp2p: invalid port\n");
                return 2;
            }
        } else if (strcmp(arg, "-t") == 0 ||
            strcmp(arg, "--transport") == 0)
        {
            if (++i >= argc) {
                fprintf(stderr, "webp2p: --transport requires a value\n");
                return 2;
            }
            transport = argv[i];
        } else if (strcmp(arg, "--pass") == 0) {
            if (!is_publisher) {
                fprintf(stderr, "webp2p: --pass is only valid for pub\n");
                return 2;
            }
            if (++i >= argc) return 2;
            options.pass = argv[i];
        } else if (strcmp(arg, "--stun") == 0) {
            if (++i >= argc) return 2;
            options.stun = argv[i];
        } else if (strcmp(arg, "--turn") == 0) {
            if (++i >= argc) return 2;
            options.turn = argv[i];
        } else if (strcmp(arg, "--turn-user") == 0) {
            if (++i >= argc) return 2;
            options.turn_user = argv[i];
        } else if (strcmp(arg, "--turn-pass") == 0) {
            if (++i >= argc) return 2;
            options.turn_pass = argv[i];
        } else if (strcmp(arg, "--force-turn") == 0) {
            if (++i >= argc || !webp2p_parse_bool(argv[i],
                &options.force_turn))
            {
                fprintf(stderr, "webp2p: --force-turn requires 0 or 1\n");
                return 2;
            }
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            webp2p_usage(argv[0]);
            return 0;
        } else if (strcmp(arg, "-v") == 0 ||
            strcmp(arg, "--version") == 0)
        {
            printf("webp2p %s\n", WEBP2P_BUILD_VERSION);
            return 0;
        } else {
            fprintf(stderr, "webp2p: unknown option: %s\n", arg);
            return 2;
        }
    }
    if (!spec || !options.port || strcmp(transport, "tcp") != 0 ||
        !webp2p_parse_spec(spec, id, index))
    {
        fprintf(stderr, "webp2p: invalid command arguments\n");
        return 2;
    }
    if ((options.turn_user && !options.turn_pass) ||
        (!options.turn_user && options.turn_pass) ||
        ((options.turn_user || options.turn_pass) && !options.turn) ||
        (options.force_turn && !options.turn))
    {
        fprintf(stderr, "webp2p: TURN options require a TURN URL and paired credentials\n");
        return 2;
    }
    options.id = id;
    options.index = index;
    signal(SIGINT, webp2p_signal);
    signal(SIGTERM, webp2p_signal);
    if (webp2p_protocol_init() != 0) {
        fprintf(stderr, "webp2p: HTTP initialization failed\n");
        return 1;
    }
    if (webp2p_rtc_init() != 0) {
        fprintf(stderr, "webp2p: RTC initialization failed\n");
        webp2p_protocol_cleanup();
        return 1;
    }
    result = is_publisher ? webp2p_publisher_run(&options) :
        webp2p_consumer_run(&options);
    webp2p_rtc_cleanup();
    webp2p_protocol_cleanup();
    return result;
}
