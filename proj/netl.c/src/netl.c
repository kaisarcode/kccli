/**
 * netl.c - Incoming network listener CLI.
 * Summary: Dispatches TCP connections or UDP datagrams to local commands.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "libnetl.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#endif

#define NETL_CLI_BUF 4096

static volatile sig_atomic_t netl_cli_stop = 0;

typedef struct netl_cli_session netl_cli_session_t;

typedef struct {
    const char *command;
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
    netl_cli_session_t *sessions;
} netl_cli_state_t;

struct netl_cli_session {
    kc_netl_peer_t *peer;
    netl_cli_state_t *state;
    int active;
    int output_finished;
#ifdef _WIN32
    HANDLE process;
    HANDLE input_write;
    HANDLE output_read;
    HANDLE output_thread;
#else
    pid_t pid;
    int input_fd;
    int output_fd;
    pthread_t output_thread;
#endif
    netl_cli_session_t *next;
};

/**
 * Lock session state.
 * @param state CLI runtime state.
 * @return None.
 */
static void netl_cli_lock(netl_cli_state_t *state)
{
#ifdef _WIN32
    EnterCriticalSection(&state->lock);
#else
    (void)pthread_mutex_lock(&state->lock);
#endif
}

/**
 * Unlock session state.
 * @param state CLI runtime state.
 * @return None.
 */
static void netl_cli_unlock(netl_cli_state_t *state)
{
#ifdef _WIN32
    LeaveCriticalSection(&state->lock);
#else
    (void)pthread_mutex_unlock(&state->lock);
#endif
}

/**
 * Print command usage.
 * @return None.
 */
static void netl_cli_help(void)
{
    printf("Usage: netl <port> <command...> [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -t, --transport <type>  tcp or udp (default: tcp)\n");
    printf("  -h, --help              Show this help\n");
    printf("  -v, --version           Show version\n");
}

/**
 * Print linked libnetl build version.
 * @return None.
 */
static void netl_cli_version(void)
{
    printf("netl build %llu\n", (unsigned long long)kc_netl_version());
}

/**
 * Parse one nonzero TCP/UDP port.
 * @param text Port text.
 * @param out Parsed port.
 * @return Nonzero on success.
 */
static int netl_cli_port(const char *text, unsigned short *out)
{
    unsigned long value;
    char *end;

    if (!text || !text[0] || !out) return 0;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno || *end != '\0' || value == 0UL || value > 65535UL) return 0;
    *out = (unsigned short)value;
    return 1;
}

/**
 * Append one positional command argument.
 * @param out Command buffer.
 * @param cap Buffer capacity.
 * @param value Argument text.
 * @return Zero on success, nonzero on overflow.
 */
static int netl_cli_append(char *out, size_t cap, const char *value)
{
    size_t used;
    size_t size;

    used = strlen(out);
    size = strlen(value);
    if (used != 0U) {
        if (used + 1U >= cap) return 1;
        out[used++] = ' ';
        out[used] = '\0';
    }
    if (size >= cap - used) return 1;
    memcpy(out + used, value, size + 1U);
    return 0;
}

/**
 * Record a termination signal.
 * @param sig Signal number.
 * @return None.
 */
static void netl_cli_signal(int sig)
{
    (void)sig;
    netl_cli_stop = 1;
}

#ifndef _WIN32
/**
 * Reap completed command processes.
 * @param sig Signal number.
 * @return None.
 */
static void netl_cli_reap(int sig)
{
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }
}
#endif

#ifdef _WIN32
/**
 * Handle Windows console termination.
 * @param type Console event type.
 * @return Nonzero when handled.
 */
static BOOL WINAPI netl_cli_console(DWORD type)
{
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT ||
            type == CTRL_CLOSE_EVENT || type == CTRL_SHUTDOWN_EVENT) {
        netl_cli_stop = 1;
        return TRUE;
    }
    return FALSE;
}
#endif

/**
 * Sleep briefly while the listener remains active.
 * @return None.
 */
static void netl_cli_sleep(void)
{
#ifdef _WIN32
    Sleep(200);
#else
    struct timespec delay;

    delay.tv_sec = 0;
    delay.tv_nsec = 200000000L;
    (void)nanosleep(&delay, NULL);
#endif
}

/**
 * Find one TCP command session.
 * Caller must hold the state lock.
 * @param state CLI runtime state.
 * @param peer Network peer.
 * @return Session pointer, or NULL.
 */
static netl_cli_session_t *netl_cli_find(
    netl_cli_state_t *state,
    kc_netl_peer_t *peer
)
{
    netl_cli_session_t *session;

    for (session = state->sessions; session; session = session->next) {
        if (session->peer == peer) return session;
    }
    return NULL;
}

/**
 * Remove one session from the state list.
 * Caller must hold the state lock.
 * @param state CLI runtime state.
 * @param target Session to remove.
 * @return None.
 */
static void netl_cli_unlink(
    netl_cli_state_t *state,
    netl_cli_session_t *target
)
{
    netl_cli_session_t **cursor;

    cursor = &state->sessions;
    while (*cursor) {
        if (*cursor == target) {
            *cursor = target->next;
            return;
        }
        cursor = &(*cursor)->next;
    }
}

#ifdef _WIN32
/**
 * Relay command stdout to one TCP peer.
 * @param arg Session pointer.
 * @return Thread result.
 */
static DWORD WINAPI netl_cli_tcp_output(LPVOID arg)
{
    netl_cli_session_t *session;
    char buffer[8192];
    DWORD amount;
    int release;

    session = (netl_cli_session_t *)arg;
    while (ReadFile(
            session->output_read,
            buffer,
            (DWORD)sizeof(buffer),
            &amount,
            NULL
        ) && amount != 0U) {
        netl_cli_lock(session->state);
        if (!session->active ||
                kc_netl_respond(session->peer, buffer, (size_t)amount) !=
                KC_NETL_OK) {
            netl_cli_unlock(session->state);
            break;
        }
        netl_cli_unlock(session->state);
    }

    WaitForSingleObject(session->process, INFINITE);
    netl_cli_lock(session->state);
    session->output_finished = 1;
    release = !session->active;
    if (release) netl_cli_unlink(session->state, session);
    netl_cli_unlock(session->state);

    CloseHandle(session->output_read);
    CloseHandle(session->process);
    if (release) free(session);
    return 0;
}

/**
 * Start one command for one TCP peer.
 * @param state CLI runtime state.
 * @param peer Network peer.
 * @return Session pointer, or NULL.
 */
static netl_cli_session_t *netl_cli_tcp_start(
    netl_cli_state_t *state,
    kc_netl_peer_t *peer
)
{
    SECURITY_ATTRIBUTES attributes;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    HANDLE input_read;
    HANDLE input_write;
    HANDLE output_read;
    HANDLE output_write;
    netl_cli_session_t *session;
    char command[NETL_CLI_BUF];

    memset(&attributes, 0, sizeof(attributes));
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    input_read = NULL;
    input_write = NULL;
    output_read = NULL;
    output_write = NULL;

    if (!CreatePipe(&input_read, &input_write, &attributes, 0)) return NULL;
    if (!CreatePipe(&output_read, &output_write, &attributes, 0)) {
        CloseHandle(input_read);
        CloseHandle(input_write);
        return NULL;
    }
    SetHandleInformation(input_write, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input_read;
    startup.hStdOutput = output_write;
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    snprintf(command, sizeof(command), "%s", state->command);

    if (!CreateProcessA(
            NULL,
            command,
            NULL,
            NULL,
            TRUE,
            CREATE_NO_WINDOW,
            NULL,
            NULL,
            &startup,
            &process
        )) {
        CloseHandle(input_read);
        CloseHandle(input_write);
        CloseHandle(output_read);
        CloseHandle(output_write);
        return NULL;
    }

    CloseHandle(process.hThread);
    CloseHandle(input_read);
    CloseHandle(output_write);
    session = (netl_cli_session_t *)calloc(1, sizeof(*session));
    if (!session) {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hProcess);
        CloseHandle(input_write);
        CloseHandle(output_read);
        return NULL;
    }

    session->peer = peer;
    session->state = state;
    session->active = 1;
    session->process = process.hProcess;
    session->input_write = input_write;
    session->output_read = output_read;
    session->output_thread = CreateThread(
        NULL,
        0,
        netl_cli_tcp_output,
        session,
        0,
        NULL
    );
    if (!session->output_thread) {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hProcess);
        CloseHandle(input_write);
        CloseHandle(output_read);
        free(session);
        return NULL;
    }
    CloseHandle(session->output_thread);
    session->output_thread = NULL;

    netl_cli_lock(state);
    session->next = state->sessions;
    state->sessions = session;
    netl_cli_unlock(state);
    return session;
}

/**
 * Dispatch one UDP datagram to a command.
 * @param state CLI runtime state.
 * @param data Datagram bytes.
 * @param size Datagram size.
 * @return Zero on success, nonzero on failure.
 */
static int netl_cli_udp_start(
    netl_cli_state_t *state,
    const void *data,
    size_t size
)
{
    SECURITY_ATTRIBUTES attributes;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    HANDLE input_read;
    HANDLE input_write;
    char command[NETL_CLI_BUF];
    DWORD written;

    memset(&attributes, 0, sizeof(attributes));
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    input_read = NULL;
    input_write = NULL;
    written = 0U;

    if (!CreatePipe(&input_read, &input_write, &attributes, 0)) return 1;
    SetHandleInformation(input_write, HANDLE_FLAG_INHERIT, 0);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input_read;
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    snprintf(command, sizeof(command), "%s", state->command);

    if (!CreateProcessA(
            NULL,
            command,
            NULL,
            NULL,
            TRUE,
            CREATE_NO_WINDOW,
            NULL,
            NULL,
            &startup,
            &process
        )) {
        CloseHandle(input_read);
        CloseHandle(input_write);
        return 1;
    }

    CloseHandle(process.hThread);
    CloseHandle(input_read);
    if (size != 0U) {
        if (!WriteFile(input_write, data, (DWORD)size, &written, NULL) ||
                written != (DWORD)size) {
            CloseHandle(input_write);
            CloseHandle(process.hProcess);
            return 1;
        }
    }
    CloseHandle(input_write);
    CloseHandle(process.hProcess);
    return 0;
}
#else
/**
 * Relay command stdout to one TCP peer.
 * @param arg Session pointer.
 * @return Thread result.
 */
static void *netl_cli_tcp_output(void *arg)
{
    netl_cli_session_t *session;
    char buffer[8192];
    ssize_t amount;
    int release;

    session = (netl_cli_session_t *)arg;
    while ((amount = read(session->output_fd, buffer, sizeof(buffer))) > 0) {
        netl_cli_lock(session->state);
        if (!session->active ||
                kc_netl_respond(session->peer, buffer, (size_t)amount) !=
                KC_NETL_OK) {
            netl_cli_unlock(session->state);
            break;
        }
        netl_cli_unlock(session->state);
    }

    (void)waitpid(session->pid, NULL, 0);
    netl_cli_lock(session->state);
    session->output_finished = 1;
    release = !session->active;
    if (release) netl_cli_unlink(session->state, session);
    netl_cli_unlock(session->state);

    (void)close(session->output_fd);
    if (release) free(session);
    return NULL;
}

/**
 * Start one command for one TCP peer.
 * @param state CLI runtime state.
 * @param peer Network peer.
 * @return Session pointer, or NULL.
 */
static netl_cli_session_t *netl_cli_tcp_start(
    netl_cli_state_t *state,
    kc_netl_peer_t *peer
)
{
    int input_pipe[2];
    int output_pipe[2];
    pid_t pid;
    netl_cli_session_t *session;

    if (pipe(input_pipe) != 0) return NULL;
    if (pipe(output_pipe) != 0) {
        (void)close(input_pipe[0]);
        (void)close(input_pipe[1]);
        return NULL;
    }

    pid = fork();
    if (pid < 0) {
        (void)close(input_pipe[0]);
        (void)close(input_pipe[1]);
        (void)close(output_pipe[0]);
        (void)close(output_pipe[1]);
        return NULL;
    }
    if (pid == 0) {
        (void)close(input_pipe[1]);
        (void)close(output_pipe[0]);
        if (dup2(input_pipe[0], STDIN_FILENO) < 0 ||
                dup2(output_pipe[1], STDOUT_FILENO) < 0) {
            _exit(1);
        }
        if (input_pipe[0] > STDERR_FILENO) (void)close(input_pipe[0]);
        if (output_pipe[1] > STDERR_FILENO) (void)close(output_pipe[1]);
        execl("/bin/sh", "sh", "-c", state->command, (char *)NULL);
        _exit(1);
    }

    (void)close(input_pipe[0]);
    (void)close(output_pipe[1]);
    session = (netl_cli_session_t *)calloc(1, sizeof(*session));
    if (!session) {
        (void)close(input_pipe[1]);
        (void)close(output_pipe[0]);
        (void)kill(pid, SIGTERM);
        (void)waitpid(pid, NULL, 0);
        return NULL;
    }

    session->peer = peer;
    session->state = state;
    session->active = 1;
    session->pid = pid;
    session->input_fd = input_pipe[1];
    session->output_fd = output_pipe[0];
    if (pthread_create(
            &session->output_thread,
            NULL,
            netl_cli_tcp_output,
            session
        ) != 0) {
        (void)close(session->input_fd);
        (void)close(session->output_fd);
        (void)kill(pid, SIGTERM);
        (void)waitpid(pid, NULL, 0);
        free(session);
        return NULL;
    }
    (void)pthread_detach(session->output_thread);

    netl_cli_lock(state);
    session->next = state->sessions;
    state->sessions = session;
    netl_cli_unlock(state);
    return session;
}

/**
 * Dispatch one UDP datagram to a command.
 * @param state CLI runtime state.
 * @param data Datagram bytes.
 * @param size Datagram size.
 * @return Zero on success, nonzero on failure.
 */
static int netl_cli_udp_start(
    netl_cli_state_t *state,
    const void *data,
    size_t size
)
{
    int pipefd[2];
    pid_t pid;
    size_t offset;

    if (pipe(pipefd) != 0) return 1;
    pid = fork();
    if (pid < 0) {
        (void)close(pipefd[0]);
        (void)close(pipefd[1]);
        return 1;
    }
    if (pid == 0) {
        (void)close(pipefd[1]);
        if (dup2(pipefd[0], STDIN_FILENO) < 0) _exit(1);
        if (pipefd[0] > STDERR_FILENO) (void)close(pipefd[0]);
        execl("/bin/sh", "sh", "-c", state->command, (char *)NULL);
        _exit(1);
    }

    (void)close(pipefd[0]);
    offset = 0U;
    while (offset < size) {
        ssize_t written;

        written = write(
            pipefd[1],
            (const unsigned char *)data + offset,
            size - offset
        );
        if (written <= 0) break;
        offset += (size_t)written;
    }
    (void)close(pipefd[1]);
    return offset == size ? 0 : 1;
}
#endif

/**
 * Deliver incoming network data to the configured command.
 * @param input Network input.
 * @param userdata CLI runtime state.
 * @return None.
 */
static void netl_cli_receive(const kc_netl_input_t *input, void *userdata)
{
    netl_cli_state_t *state;
    netl_cli_session_t *session;

    state = (netl_cli_state_t *)userdata;
    if (!input || !state) return;
    if (input->protocol == KC_NETL_UDP) {
        (void)netl_cli_udp_start(state, input->data, input->data_size);
        return;
    }

    netl_cli_lock(state);
    session = netl_cli_find(state, input->peer);
    netl_cli_unlock(state);
    if (!session) session = netl_cli_tcp_start(state, input->peer);
    if (!session) {
        kc_netl_peer_close(input->peer);
        return;
    }

#ifdef _WIN32
    {
        DWORD written;
        size_t offset;

        offset = 0U;
        while (offset < input->data_size) {
            if (!WriteFile(
                    session->input_write,
                    (const unsigned char *)input->data + offset,
                    (DWORD)(input->data_size - offset),
                    &written,
                    NULL
                ) || written == 0U) {
                kc_netl_peer_close(input->peer);
                return;
            }
            offset += (size_t)written;
        }
    }
#else
    {
        size_t offset;

        offset = 0U;
        while (offset < input->data_size) {
            ssize_t written;

            written = write(
                session->input_fd,
                (const unsigned char *)input->data + offset,
                input->data_size - offset
            );
            if (written <= 0) {
                kc_netl_peer_close(input->peer);
                return;
            }
            offset += (size_t)written;
        }
    }
#endif
}

/**
 * Close the command input associated with one TCP peer.
 * @param peer Closed network peer.
 * @param userdata CLI runtime state.
 * @return None.
 */
static void netl_cli_disconnect(kc_netl_peer_t *peer, void *userdata)
{
    netl_cli_state_t *state;
    netl_cli_session_t *session;
    int release;

    state = (netl_cli_state_t *)userdata;
    if (!state) return;
    netl_cli_lock(state);
    session = netl_cli_find(state, peer);
    if (!session) {
        netl_cli_unlock(state);
        return;
    }
    session->active = 0;
#ifdef _WIN32
    if (session->input_write) {
        CloseHandle(session->input_write);
        session->input_write = NULL;
    }
#else
    if (session->input_fd >= 0) {
        (void)close(session->input_fd);
        session->input_fd = -1;
    }
#endif
    release = session->output_finished;
    if (release) netl_cli_unlink(state, session);
    netl_cli_unlock(state);
    if (release) free(session);
}

/**
 * Stop after one asynchronous listener error.
 * @param status libnetl status.
 * @param userdata Unused CLI state.
 * @return None.
 */
static void netl_cli_error(int status, void *userdata)
{
    (void)status;
    (void)userdata;
    netl_cli_stop = 1;
}

/**
 * Release remaining command sessions after listener close.
 * @param state CLI runtime state.
 * @return None.
 */
static void netl_cli_cleanup(netl_cli_state_t *state)
{
    netl_cli_session_t *session;

    netl_cli_lock(state);
    session = state->sessions;
    state->sessions = NULL;
    while (session) {
        netl_cli_session_t *next;

        next = session->next;
        session->active = 0;
#ifdef _WIN32
        if (session->input_write) CloseHandle(session->input_write);
        if (session->process) TerminateProcess(session->process, 0);
#else
        if (session->input_fd >= 0) (void)close(session->input_fd);
        if (session->pid > 0) (void)kill(session->pid, SIGTERM);
#endif
        session = next;
    }
    netl_cli_unlock(state);
}

/**
 * Execute the command-line interface.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process status.
 */
int main(int argc, char **argv)
{
    kc_netl_options_t options;
    kc_netl_t *listener;
    netl_cli_state_t state;
    unsigned short port;
    char command[NETL_CLI_BUF];
    int protocol;
    int positional;
    int i;
    int rc;

    if (argc == 2 &&
            (strcmp(argv[1], "-h") == 0 ||
            strcmp(argv[1], "--help") == 0)) {
        netl_cli_help();
        return 0;
    }
    if (argc == 2 &&
            (strcmp(argv[1], "-v") == 0 ||
            strcmp(argv[1], "--version") == 0)) {
        netl_cli_version();
        return 0;
    }

    port = 0U;
    command[0] = '\0';
    protocol = KC_NETL_TCP;
    positional = 0;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 ||
                strcmp(argv[i], "--help") == 0) {
            netl_cli_help();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 ||
                strcmp(argv[i], "--version") == 0) {
            netl_cli_version();
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 ||
                strcmp(argv[i], "--transport") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "netl: --transport requires tcp or udp\n");
                return 1;
            }
            if (strcmp(argv[i], "tcp") == 0) {
                protocol = KC_NETL_TCP;
            } else if (strcmp(argv[i], "udp") == 0) {
                protocol = KC_NETL_UDP;
            } else {
                fprintf(stderr, "netl: --transport requires tcp or udp\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "netl: unknown option: %s\n", argv[i]);
            return 1;
        } else if (positional == 0) {
            if (!netl_cli_port(argv[i], &port)) {
                fprintf(stderr, "netl: invalid port: %s\n", argv[i]);
                return 1;
            }
            positional = 1;
        } else {
            if (netl_cli_append(command, sizeof(command), argv[i]) != 0) {
                fprintf(stderr, "netl: command is too long\n");
                return 1;
            }
            positional++;
        }
    }

    if (port == 0U) {
        fprintf(stderr, "netl: missing port\n");
        return 1;
    }
    if (!command[0]) {
        fprintf(stderr, "netl: missing command\n");
        return 1;
    }

    memset(&state, 0, sizeof(state));
    state.command = command;
#ifdef _WIN32
    InitializeCriticalSection(&state.lock);
    (void)SetConsoleCtrlHandler(netl_cli_console, TRUE);
#else
    if (pthread_mutex_init(&state.lock, NULL) != 0) return 1;
    (void)signal(SIGINT, netl_cli_signal);
    (void)signal(SIGTERM, netl_cli_signal);
    (void)signal(SIGPIPE, SIG_IGN);
    (void)signal(SIGCHLD, netl_cli_reap);
#endif

    memset(&options, 0, sizeof(options));
    options.port = port;
    options.protocol = protocol;
    listener = NULL;
    rc = kc_netl_open(
        &listener,
        &options,
        netl_cli_receive,
        netl_cli_disconnect,
        netl_cli_error,
        &state
    );
    if (rc != KC_NETL_OK) {
        fprintf(stderr, "netl: %s\n", kc_netl_strerror(rc));
        return 1;
    }

    while (!netl_cli_stop) netl_cli_sleep();
    kc_netl_close(listener);
    netl_cli_cleanup(&state);
    return 0;
}
