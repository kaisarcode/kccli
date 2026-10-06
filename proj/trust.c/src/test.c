/**
 * test.c - trust CLI contract tests.
 * Summary: Validates trust establishment, message protection, and revocation.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef KC_CLI_PATH
#define KC_CLI_PATH "trust"
#endif

static int test_total = 0;
static int test_failed = 0;
static char store_a[256];
static char store_b[256];

/**
 * Prints one contract-test result.
 * @param name Test name.
 * @param detail Behavior under test.
 * @param ok Nonzero on success.
 * @return None.
 */
static void result(const char *name, const char *detail, int ok)
{
    test_total++;
    if (!ok) test_failed++;
    printf("[%d/10] [%s] %s: %s\n", test_total, ok ? "PASS" : "FAIL",
        name, detail);
}

/**
 * Writes exact fixture bytes.
 * @param path Destination path.
 * @param data Source bytes.
 * @param size Byte count.
 * @return Nonzero on success.
 */
static int write_file(const char *path, const void *data, size_t size)
{
    FILE *file = fopen(path, "wb");
    int ok;

    if (!file) return 0;
    ok = fwrite(data, 1, size, file) == size;
    fclose(file);
    return ok;
}

/**
 * Reads one file into a fixed buffer.
 * @param path Source path.
 * @param buffer Destination buffer.
 * @param capacity Buffer capacity.
 * @param out_size Receives byte count.
 * @return Nonzero on success.
 */
static int read_file(
    const char *path,
    unsigned char *buffer,
    size_t capacity,
    size_t *out_size
)
{
    FILE *file = fopen(path, "rb");
    size_t size;

    if (!file) return 0;
    size = fread(buffer, 1, capacity, file);
    if (ferror(file)) {
        fclose(file);
        return 0;
    }
    if (fgetc(file) != EOF) {
        fclose(file);
        return 0;
    }
    fclose(file);
    *out_size = size;
    return 1;
}

/**
 * Executes the CLI in one isolated trust store.
 * @param store Store directory.
 * @param args CLI arguments.
 * @param input Optional input file.
 * @param output Optional output file.
 * @return Process status from system().
 */
static int run_cli(
    const char *store,
    const char *args,
    const char *input,
    const char *output
)
{
    char command[16384];

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "set \"KC_TRUST_DIR=%s\" && \"%s\" %s %s %s 2>NUL",
        store, KC_CLI_PATH, args ? args : "",
        input ? "< trust-cli-input.bin" : "",
        output ? "> trust-cli-output.bin" : "> NUL");
#else
    snprintf(command, sizeof(command),
        "KC_TRUST_DIR=\"%s\" \"%s\" %s %s %s 2>/dev/null",
        store, KC_CLI_PATH, args ? args : "",
        input ? "< trust-cli-input.bin" : "",
        output ? "> trust-cli-output.bin" : "> /dev/null");
#endif

    return system(command);
}

/**
 * Captures text stdout from one CLI call.
 * @param store Store directory.
 * @param args CLI arguments.
 * @param buffer Destination text buffer.
 * @param capacity Buffer capacity.
 * @return Nonzero on success.
 */
static int capture_text(
    const char *store,
    const char *args,
    char *buffer,
    size_t capacity
)
{
    unsigned char bytes[8192];
    size_t size;

    if (run_cli(store, args, NULL, "trust-cli-output.bin") != 0) return 0;
    if (!read_file("trust-cli-output.bin", bytes, sizeof(bytes) - 1, &size)) {
        remove("trust-cli-output.bin");
        return 0;
    }
    remove("trust-cli-output.bin");
    if (size >= capacity) return 0;
    memcpy(buffer, bytes, size);
    buffer[size] = '\0';
    while (size > 0 && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r')) {
        buffer[--size] = '\0';
    }
    return 1;
}

/**
 * Splits one TSV line into two fields.
 * @param line Mutable input line.
 * @param first Receives first field.
 * @param second Receives second field.
 * @return Nonzero on success.
 */
static int split_tsv(char *line, char **first, char **second)
{
    char *tab = strchr(line, '\t');

    if (!tab || strchr(tab + 1, '\t')) return 0;
    *tab = '\0';
    *first = line;
    *second = tab + 1;
    return **first != '\0' && **second != '\0';
}

/**
 * Removes test state and temporary files.
 * @return None.
 */
static void cleanup(void)
{
    char command[1024];

#ifdef _WIN32
    snprintf(command, sizeof(command),
        "rmdir /S /Q \"%s\" 2>NUL & rmdir /S /Q \"%s\" 2>NUL",
        store_a, store_b);
#else
    snprintf(command, sizeof(command),
        "rm -rf \"%s\" \"%s\"", store_a, store_b);
#endif
    system(command);
    remove("trust-cli-input.bin");
    remove("trust-cli-output.bin");
}

/**
 * Runs all trust CLI contract tests.
 * @return Zero when all tests pass.
 */
int main(void)
{
    char invite_line[8192] = {0};
    char join_line[8192] = {0};
    char confirm_uid[256] = {0};
    char invite_copy[8192];
    char join_copy[8192];
    char *alice_uid;
    char *code;
    char *bob_uid;
    char *confirmation;
    char args[12288];
    unsigned char recovered[256];
    size_t recovered_size = 0;
    static const unsigned char payload[] = {0, 1, 2, 3, 0, 255, 10, 11};
#ifdef _WIN32
    unsigned long pid = (unsigned long)GetCurrentProcessId();
#else
    unsigned long pid = (unsigned long)getpid();
#endif

    snprintf(store_a, sizeof(store_a), "trust-cli-a-%lu", pid);
    snprintf(store_b, sizeof(store_b), "trust-cli-b-%lu", pid);
    cleanup();

    result("help", "--help succeeds", run_cli(store_a, "--help", NULL, NULL) == 0);
    result("version", "--version succeeds",
        run_cli(store_a, "--version", NULL, NULL) == 0);
    result("unknown", "unknown command fails",
        run_cli(store_a, "unknown", NULL, NULL) != 0);

    result("invite", "invite returns uid and code as TSV",
        capture_text(store_a, "invite", invite_line, sizeof(invite_line)) &&
        strlen(invite_line) > 38 && strchr(invite_line, '\t') != NULL);

    snprintf(invite_copy, sizeof(invite_copy), "%s", invite_line);
    if (!split_tsv(invite_copy, &alice_uid, &code)) {
        alice_uid = NULL;
        code = NULL;
    }

    if (code) {
        snprintf(args, sizeof(args), "join \"%s\"", code);
    } else {
        snprintf(args, sizeof(args), "join invalid");
    }
    result("join", "join returns inviter uid and confirmation as TSV",
        capture_text(store_b, args, join_line, sizeof(join_line)) &&
        strchr(join_line, '\t') != NULL);

    snprintf(join_copy, sizeof(join_copy), "%s", join_line);
    if (!split_tsv(join_copy, &bob_uid, &confirmation)) {
        bob_uid = NULL;
        confirmation = NULL;
    }

    if (confirmation) {
        snprintf(args, sizeof(args), "confirm \"%s\"", confirmation);
    } else {
        snprintf(args, sizeof(args), "confirm invalid");
    }
    result("confirm", "confirm returns the invited endpoint uid",
        capture_text(store_a, args, confirm_uid, sizeof(confirm_uid)) &&
        alice_uid && strcmp(confirm_uid, alice_uid) == 0);

    result("uids", "join and invite endpoint uids differ",
        alice_uid && bob_uid && strcmp(alice_uid, bob_uid) != 0);

    if (!write_file("trust-cli-input.bin", payload, sizeof(payload))) {
        cleanup();
        return 1;
    }
    if (alice_uid) snprintf(args, sizeof(args), "seal \"%s\"", alice_uid);
    else snprintf(args, sizeof(args), "seal invalid");
    result("seal", "seal accepts binary stdin",
        run_cli(store_a, args, "trust-cli-input.bin", "trust-cli-output.bin") == 0);

    remove("trust-cli-input.bin");
    rename("trust-cli-output.bin", "trust-cli-input.bin");
    if (alice_uid) snprintf(args, sizeof(args), "unseal \"%s\"", alice_uid);
    else snprintf(args, sizeof(args), "unseal invalid");
    result("unseal", "unseal restores exact binary plaintext",
        run_cli(store_b, args, "trust-cli-input.bin", "trust-cli-output.bin") == 0 &&
        read_file("trust-cli-output.bin", recovered, sizeof(recovered),
            &recovered_size) &&
        recovered_size == sizeof(payload) &&
        memcmp(recovered, payload, sizeof(payload)) == 0);

    if (alice_uid) snprintf(args, sizeof(args), "revoke \"%s\"", alice_uid);
    else snprintf(args, sizeof(args), "revoke invalid");
    result("revoke", "revoke removes the relationship",
        run_cli(store_a, args, NULL, NULL) == 0 &&
        run_cli(store_a, args, NULL, NULL) != 0);

    cleanup();
    printf("\n%d passed, %d failed\n", test_total - test_failed, test_failed);
    return test_failed ? 1 : 0;
}
