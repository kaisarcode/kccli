/**
 * entry.c - REDP2P CLI argument entry point.
 * Summary: Normalizes public CLI arguments before dispatch.
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

#define main redp2p_cli_main
#include "redp2p.c"
#undef main

/**
 * Sets one REDP2P environment override for the delegated CLI.
 * @param name Environment variable name.
 * @param value Environment variable value.
 * @return 1 on success, 0 on failure.
 */
static int redp2p_cli_setenv(const char *name, const char *value)
{
#ifdef _WIN32
    return _putenv_s(name, value) == 0;
#else
    return setenv(name, value, 1) == 0;
#endif
}

/**
 * Appends one VIP id/password pair to the CLI override buffer.
 * @param text Current buffer, or NULL.
 * @param length Current buffer length.
 * @param id VIP identifier.
 * @param pass VIP password.
 * @return New buffer, or NULL on allocation failure.
 */
static char *redp2p_cli_vip_append(char *text, size_t *length,
    const char *id, const char *pass)
{
    size_t id_len = strlen(id);
    size_t pass_len = strlen(pass);
    size_t extra = id_len + pass_len + (*length ? 2U : 1U);
    char *next = (char *)realloc(text, *length + extra + 1U);
    size_t offset;

    if (!next) {
        free(text);
        return NULL;
    }
    offset = *length;
    if (offset) next[offset++] = ' ';
    memcpy(next + offset, id, id_len);
    offset += id_len;
    next[offset++] = ' ';
    memcpy(next + offset, pass, pass_len);
    offset += pass_len;
    next[offset] = '\0';
    *length = offset;
    return next;
}

/**
 * Prints the public REDP2P command usage.
 * @param name Program executable name.
 * @return None.
 */
static void redp2p_cli_public_usage(const char *name)
{
    printf("Usage: %s <command> [options]\n", name);
    printf("\n");
    printf("Commands:\n");
    printf("  idx <port> [options]                    Run an index\n");
    printf("  pub <id>@<index[:port]> -p <port>       Publish a local service\n");
    printf("  con <id>@<index[:port]> -p <port>       Expose a published service\n");
    printf("\n");
    printf("idx options:\n");
    printf("  --seats <N>            Publisher capacity; default unlimited\n");
    printf("  --pow <N>              Registration PoW bits (0..32)\n");
    printf("  --max-consumers <N>    Consumers allowed per publisher\n");
    printf("  --pass <pass>          Registration password\n");
    printf("  --vip <id> <pass>      Reserve an id/password pair; repeatable\n");
    printf("\n");
    printf("pub options:\n");
    printf("  -p, --port <port>      Local service port\n");
    printf("  --transport <type>     tcp or udp (default: tcp)\n");
    printf("  --pass <pass>          Index registration password\n");
    printf("  --stun <url>           Optional STUN URL\n");
    printf("  --turn <url>           Optional TURN URL\n");
    printf("  --turn-user <u>        Optional TURN username\n");
    printf("  --turn-pass <p>        Optional TURN password\n");
    printf("  --force-turn <0|1>     Disable or require TURN relay\n");
    printf("\n");
    printf("con options:\n");
    printf("  -p, --port <port>      Local service port\n");
    printf("  --transport <type>     tcp or udp (default: tcp)\n");
    printf("  --stun <url>           Optional STUN URL\n");
    printf("  --turn <url>           Optional TURN URL\n");
    printf("  --turn-user <u>        Optional TURN username\n");
    printf("  --turn-pass <p>        Optional TURN password\n");
    printf("  --force-turn <0|1>     Disable or require TURN relay\n");
    printf("\n");
    printf("Common options:\n");
    printf("  -h, --help             Show this help\n");
    printf("  -v, --version          Show version\n");
}

/**
 * Reports whether one command accepts peer transport options.
 * @param command Command name.
 * @return Nonzero for pub and con.
 */
static int redp2p_cli_peer_command(const char *command)
{
    return strcmp(command, "pub") == 0 || strcmp(command, "con") == 0;
}

/**
 * Dispatches the REDP2P CLI with normalized public arguments.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    char **next;
    char *vip = NULL;
    size_t vip_len = 0;
    const char *command;
    int transport_set = 0;
    int out;

    if (argc < 2 || strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0)
    {
        redp2p_cli_public_usage(argv[0]);
        return argc < 2 ? 1 : 0;
    }
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)
        return redp2p_cli_main(argc, argv);

    command = argv[1];
    if (strcmp(command, "idx") != 0 && strcmp(command, "pub") != 0 &&
        strcmp(command, "con") != 0)
        return redp2p_cli_main(argc, argv);

    next = (char **)calloc((size_t)argc + 1U, sizeof(*next));
    if (!next) return 1;
    next[0] = argv[0];
    next[1] = argv[1];
    out = 2;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--tcp") == 0 || strcmp(argv[i], "--udp") == 0) {
            fprintf(stderr, "redp2p: use --transport tcp|udp\n");
            free(vip);
            free(next);
            return 1;
        }
        if (strcmp(argv[i], "--transport") == 0) {
            if (!redp2p_cli_peer_command(command) || transport_set ||
                ++i >= argc)
            {
                fprintf(stderr, "redp2p: --transport requires tcp or udp\n");
                free(vip);
                free(next);
                return 1;
            }
            if (strcmp(argv[i], "tcp") == 0)
                next[out++] = "--tcp";
            else if (strcmp(argv[i], "udp") == 0)
                next[out++] = "--udp";
            else {
                fprintf(stderr, "redp2p: --transport requires tcp or udp\n");
                free(vip);
                free(next);
                return 1;
            }
            transport_set = 1;
            continue;
        }
        if (strcmp(argv[i], "--pass") == 0) {
            if (strcmp(command, "con") == 0 || ++i >= argc || !argv[i][0] ||
                !redp2p_cli_setenv("REDP2P_PASS", argv[i]))
            {
                fprintf(stderr, "redp2p: --pass requires a value for idx/pub\n");
                free(vip);
                free(next);
                return 1;
            }
            continue;
        }
        if (strcmp(argv[i], "--vip") == 0) {
            if (strcmp(command, "idx") != 0 || i + 2 >= argc ||
                !argv[i + 1][0] || !argv[i + 2][0])
            {
                fprintf(stderr, "redp2p: --vip requires <id> <pass> for idx\n");
                free(vip);
                free(next);
                return 1;
            }
            vip = redp2p_cli_vip_append(vip, &vip_len,
                argv[i + 1], argv[i + 2]);
            if (!vip) {
                free(next);
                return 1;
            }
            i += 2;
            continue;
        }
        if (strcmp(argv[i], "--force-turn") == 0) {
            if (!redp2p_cli_peer_command(command) || ++i >= argc ||
                (strcmp(argv[i], "0") != 0 && strcmp(argv[i], "1") != 0) ||
                !redp2p_cli_setenv("REDP2P_FORCE_TURN", argv[i]))
            {
                fprintf(stderr, "redp2p: --force-turn requires 0 or 1\n");
                free(vip);
                free(next);
                return 1;
            }
            continue;
        }
        next[out++] = argv[i];
    }

    if (vip && !redp2p_cli_setenv("REDP2P_VIP", vip)) {
        free(vip);
        free(next);
        return 1;
    }
    free(vip);
    next[out] = NULL;
    out = redp2p_cli_main(out, next);
    free(next);
    return out;
}
