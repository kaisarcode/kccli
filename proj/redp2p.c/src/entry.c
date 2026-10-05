/**
 * entry.c - REDP2P CLI argument entry point.
 * Summary: Normalizes public CLI transport arguments before dispatch.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define main redp2p_cli_main
#include "redp2p.c"
#undef main

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
    printf("  idx <port> [--seats <N>] [--pow <N>] [--max-consumers <N>]\n");
    printf("  pub <id>@<index[:port]> -p <port> [peer options]\n");
    printf("  con <id>@<index[:port]> -p <port> [peer options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  -p, --port <port>    Local service port for pub/con\n");
    printf("  --transport <type>   tcp or udp (default: tcp)\n");
    printf("  -h, --help           Show this help\n");
    printf("  -v, --version        Show version\n");
    printf("  --stun <url>         Optional STUN URL\n");
    printf("  --turn <url>         Optional TURN URL\n");
    printf("  --turn-user <u>      Optional TURN username\n");
    printf("  --turn-pass <p>      Optional TURN password\n");
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
    printf("  REDP2P_FORCE_TURN     1 forces pub/con traffic through TURN; 0 disables\n");
}

/**
 * Dispatches the REDP2P CLI with normalized transport arguments.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    char **next;
    int transport_set;
    int out;

    if (argc < 2 || strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0)
    {
        redp2p_cli_public_usage(argv[0]);
        return argc < 2 ? 1 : 0;
    }
    if (strcmp(argv[1], "pub") != 0 && strcmp(argv[1], "con") != 0)
        return redp2p_cli_main(argc, argv);

    next = (char **)calloc((size_t)argc + 1U, sizeof(*next));
    if (!next) return 1;
    next[0] = argv[0];
    next[1] = argv[1];
    transport_set = 0;
    out = 2;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--tcp") == 0 || strcmp(argv[i], "--udp") == 0) {
            fprintf(stderr, "redp2p: use --transport tcp|udp\n");
            free(next);
            return 1;
        }
        if (strcmp(argv[i], "--transport") == 0) {
            if (transport_set || ++i >= argc) {
                fprintf(stderr, "redp2p: --transport requires tcp or udp\n");
                free(next);
                return 1;
            }
            if (strcmp(argv[i], "tcp") == 0)
                next[out++] = "--tcp";
            else if (strcmp(argv[i], "udp") == 0)
                next[out++] = "--udp";
            else {
                fprintf(stderr, "redp2p: --transport requires tcp or udp\n");
                free(next);
                return 1;
            }
            transport_set = 1;
            continue;
        }
        next[out++] = argv[i];
    }
    next[out] = NULL;
    out = redp2p_cli_main(out, next);
    free(next);
    return out;
}
