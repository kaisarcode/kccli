/**
 * trust.c - Scoped trust command-line interface.
 * Summary: Establishes trust relationships and protects binary messages.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "libtrust.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

/**
 * Prints public CLI usage.
 * @param name Program executable name.
 * @return None.
 */
static void trust_cli_usage(const char *name)
{
    printf("Usage: %s <command> [arguments] [options]\n", name);
    printf("\n");
    printf("Commands:\n");
    printf("  invite                    Create a one-use invitation\n");
    printf("  join <code>               Join an invitation\n");
    printf("  confirm <confirmation>    Confirm a joined invitation\n");
    printf("  seal <uid> [message]      Protect data for a remote UID\n");
    printf("  unseal <uid> [data]       Restore data addressed to a local UID\n");
    printf("  revoke <uid>              Revoke a relationship or invitation\n");
    printf("\n");
    printf("Options:\n");
    printf("  -i, --input <file>        Read seal/unseal input from file\n");
    printf("  -h, --help                Show this help\n");
    printf("  -v, --version             Show version\n");
}

/**
 * Reports whether standard input is attached to an interactive terminal.
 * @return Nonzero when stdin is interactive.
 */
static int trust_cli_stdin_is_tty(void)
{
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;

    if (handle == INVALID_HANDLE_VALUE || handle == NULL) return 0;
    return GetConsoleMode(handle, &mode) != 0;
#else
    return isatty(STDIN_FILENO);
#endif
}

/**
 * Reads all bytes from one stream.
 * @param stream Input stream.
 * @param out Receives allocated data.
 * @param out_size Receives byte count.
 * @return Zero on success, nonzero on failure.
 */
static int trust_cli_read_stream(FILE *stream, void **out, size_t *out_size)
{
    unsigned char *buffer = NULL;
    size_t size = 0;
    size_t capacity = 0;
    unsigned char chunk[4096];
    size_t count;

    if (!stream || !out || !out_size) return 1;
    *out = NULL;
    *out_size = 0;

    while ((count = fread(chunk, 1, sizeof(chunk), stream)) > 0) {
        size_t required;
        unsigned char *next;

        if (count > SIZE_MAX - size) {
            free(buffer);
            return 1;
        }
        required = size + count;
        if (required > capacity) {
            size_t next_capacity = capacity ? capacity : 4096;

            while (next_capacity < required) {
                if (next_capacity > SIZE_MAX / 2) {
                    next_capacity = required;
                    break;
                }
                next_capacity *= 2;
            }
            next = (unsigned char *)realloc(buffer, next_capacity);
            if (!next) {
                free(buffer);
                return 1;
            }
            buffer = next;
            capacity = next_capacity;
        }
        memcpy(buffer + size, chunk, count);
        size += count;
    }

    if (ferror(stream)) {
        free(buffer);
        return 1;
    }

    if (!buffer) {
        buffer = (unsigned char *)malloc(1);
        if (!buffer) return 1;
    }

    *out = buffer;
    *out_size = size;
    return 0;
}

/**
 * Reads all bytes from one file.
 * @param path Input file path.
 * @param out Receives allocated data.
 * @param out_size Receives byte count.
 * @return Zero on success, nonzero on failure.
 */
static int trust_cli_read_file(
    const char *path,
    void **out,
    size_t *out_size
)
{
    FILE *file;
    int status;

    if (!path) return 1;
    file = fopen(path, "rb");
    if (!file) return 1;
    status = trust_cli_read_stream(file, out, out_size);
    fclose(file);
    return status;
}

/**
 * Writes exact binary output to stdout.
 * @param data Source bytes.
 * @param size Byte count.
 * @return Zero on success, nonzero on failure.
 */
static int trust_cli_write_stdout(const void *data, size_t size)
{
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) return 1;
#endif
    if (size > 0 && fwrite(data, 1, size, stdout) != size) return 1;
    return fflush(stdout) == 0 ? 0 : 1;
}

/**
 * Resolves seal/unseal input with argument, stdin, then file precedence.
 * @param direct Optional direct argument.
 * @param input_path Optional input file.
 * @param out Receives allocated data when needed.
 * @param out_size Receives byte count.
 * @param direct_data Receives direct pointer when direct input wins.
 * @return Zero on success, nonzero on failure.
 */
static int trust_cli_resolve_input(
    const char *direct,
    const char *input_path,
    void **out,
    size_t *out_size,
    const void **direct_data
)
{
    *out = NULL;
    *out_size = 0;
    *direct_data = NULL;

    if (direct) {
        *direct_data = direct;
        *out_size = strlen(direct);
        return 0;
    }

    if (!trust_cli_stdin_is_tty()) {
#ifdef _WIN32
        if (_setmode(_fileno(stdin), _O_BINARY) == -1) return 1;
#endif
        if (trust_cli_read_stream(stdin, out, out_size)) return 1;
        if (*out_size > 0 || !input_path) return 0;
        free(*out);
        *out = NULL;
    }

    if (input_path) return trust_cli_read_file(input_path, out, out_size);

    *out = malloc(1);
    return *out ? 0 : 1;
}

/**
 * Executes invite.
 * @param trust Trust context.
 * @return Process exit code.
 */
static int trust_cli_invite(kc_trust_t *trust)
{
    char *uid = NULL;
    char *code = NULL;
    int status = 1;

    if (kc_trust_invite(trust, &uid, &code) != KC_TRUST_OK) {
        fprintf(stderr, "trust: invite failed\n");
        goto done;
    }
    if (printf("%s\t%s\n", uid, code) < 0 || fflush(stdout) != 0) goto done;
    status = 0;

done:
    kc_trust_free(uid);
    kc_trust_free(code);
    return status;
}

/**
 * Executes join.
 * @param trust Trust context.
 * @param code Invitation code.
 * @return Process exit code.
 */
static int trust_cli_join(kc_trust_t *trust, const char *code)
{
    char *uid = NULL;
    char *confirmation = NULL;
    int status = 1;

    if (kc_trust_join(trust, code, &uid, &confirmation) != KC_TRUST_OK) {
        fprintf(stderr, "trust: join failed\n");
        goto done;
    }
    if (printf("%s\t%s\n", uid, confirmation) < 0 || fflush(stdout) != 0) {
        goto done;
    }
    status = 0;

done:
    kc_trust_free(uid);
    kc_trust_free(confirmation);
    return status;
}

/**
 * Executes confirm.
 * @param trust Trust context.
 * @param confirmation Confirmation text.
 * @return Process exit code.
 */
static int trust_cli_confirm(kc_trust_t *trust, const char *confirmation)
{
    char *uid = NULL;
    int status = 1;

    if (kc_trust_confirm(trust, confirmation, &uid) != KC_TRUST_OK) {
        fprintf(stderr, "trust: confirm failed\n");
        goto done;
    }
    if (printf("%s\n", uid) < 0 || fflush(stdout) != 0) goto done;
    status = 0;

done:
    kc_trust_free(uid);
    return status;
}

/**
 * Executes seal or unseal.
 * @param trust Trust context.
 * @param unseal Nonzero for unseal.
 * @param uid Destination UID.
 * @param direct Optional direct input.
 * @param input_path Optional file input.
 * @return Process exit code.
 */
static int trust_cli_transform(
    kc_trust_t *trust,
    int unseal,
    const char *uid,
    const char *direct,
    const char *input_path
)
{
    void *owned_input = NULL;
    size_t input_size = 0;
    const void *direct_data = NULL;
    const void *input;
    void *output = NULL;
    size_t output_size = 0;
    int rc;
    int status = 1;

    if (trust_cli_resolve_input(direct, input_path, &owned_input,
        &input_size, &direct_data))
    {
        fprintf(stderr, "trust: cannot read input\n");
        return 1;
    }
    input = direct_data ? direct_data : owned_input;

    if (unseal) {
        rc = kc_trust_unseal(trust, uid, input, input_size,
            &output, &output_size);
    } else {
        rc = kc_trust_seal(trust, uid, input, input_size,
            &output, &output_size);
    }
    if (rc != KC_TRUST_OK) {
        fprintf(stderr, "trust: %s failed\n", unseal ? "unseal" : "seal");
        goto done;
    }

    status = trust_cli_write_stdout(output, output_size);

done:
    free(owned_input);
    kc_trust_free(output);
    return status;
}

/**
 * Runs the trust CLI.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    kc_trust_t *trust = NULL;
    const char *command;
    const char *input_path = NULL;
    const char *positionals[3];
    int positional_count = 0;
    int status = 1;

    if (argc == 2 && (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0))
    {
        trust_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") == 0 ||
        strcmp(argv[1], "--version") == 0))
    {
        printf("%llu\n", (unsigned long long)kc_trust_version());
        return 0;
    }
    if (argc < 2) {
        trust_cli_usage(argv[0]);
        return 1;
    }

    command = argv[1];
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            if (++i >= argc || !argv[i][0]) {
                fprintf(stderr, "trust: --input requires a file\n");
                return 1;
            }
            if (input_path) {
                fprintf(stderr, "trust: only one input file is accepted\n");
                return 1;
            }
            input_path = argv[i];
        } else if (strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-v") == 0 ||
            strcmp(argv[i], "--version") == 0)
        {
            fprintf(stderr, "trust: -h/--help and -v/--version must be used alone\n");
            return 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "trust: unknown option '%s'\n", argv[i]);
            return 1;
        } else if (positional_count < 3) {
            positionals[positional_count++] = argv[i];
        } else {
            fprintf(stderr, "trust: too many arguments\n");
            return 1;
        }
    }

    if (kc_trust_init(&trust) != KC_TRUST_OK) {
        fprintf(stderr, "trust: cannot initialize local state\n");
        return 1;
    }

    if (strcmp(command, "invite") == 0) {
        if (positional_count != 0 || input_path) {
            fprintf(stderr, "trust: invite takes no arguments\n");
        } else {
            status = trust_cli_invite(trust);
        }
    } else if (strcmp(command, "join") == 0) {
        if (positional_count != 1 || input_path) {
            fprintf(stderr, "trust: join requires one invitation code\n");
        } else {
            status = trust_cli_join(trust, positionals[0]);
        }
    } else if (strcmp(command, "confirm") == 0) {
        if (positional_count != 1 || input_path) {
            fprintf(stderr, "trust: confirm requires one confirmation\n");
        } else {
            status = trust_cli_confirm(trust, positionals[0]);
        }
    } else if (strcmp(command, "seal") == 0 || strcmp(command, "unseal") == 0) {
        if (positional_count < 1 || positional_count > 2) {
            fprintf(stderr, "trust: %s requires <uid> and optional data\n", command);
        } else {
            status = trust_cli_transform(
                trust,
                strcmp(command, "unseal") == 0,
                positionals[0],
                positional_count == 2 ? positionals[1] : NULL,
                input_path
            );
        }
    } else if (strcmp(command, "revoke") == 0) {
        if (positional_count != 1 || input_path) {
            fprintf(stderr, "trust: revoke requires one uid\n");
        } else if (kc_trust_revoke(trust, positionals[0]) != KC_TRUST_OK) {
            fprintf(stderr, "trust: revoke failed\n");
        } else {
            status = 0;
        }
    } else {
        fprintf(stderr, "trust: unknown command '%s'\n", command);
    }

    kc_trust_close(trust);
    return status;
}
