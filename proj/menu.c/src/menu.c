/**
 * menu.c - Native Application Menu CLI
 * Summary: Command-line interface backed by libmenu.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#include "libmenu.h"

#include <stdio.h>
#include <string.h>

/**
 * Print command usage.
 * @return None.
 */
static void kc_cli_help(void) {
    printf("Usage: menu <command> [arguments]\n");
    printf("\n");
    printf("Commands:\n");
    printf("  menu add <id> <command> [options]\n");
    printf("      Add an application menu entry\n");
    printf("  menu delete <id>\n");
    printf("      Delete an application menu entry\n");
    printf("\n");
    printf("Add options:\n");
    printf("  --name <name>         Application name (defaults to id)\n");
    printf("  --description <text>  Application description\n");
    printf("  --icon <path>         Application icon path\n");
    printf("  --category <name>     Application menu category\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help            Show this help\n");
    printf("  -v, --version         Show version\n");
}

/**
 * Print the linked libmenu build version.
 * @return None.
 */
static void kc_cli_version(void) {
    printf("menu build %llu\n", (unsigned long long)kc_menu_version());
}

/**
 * Add one application menu entry.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process status.
 */
static int kc_cli_add(int argc, char **argv) {
    kc_menu_entry_t entry;
    int i;

    if (argc < 4) {
        fprintf(stderr, "menu: add requires <id> <command>\n");
        return 1;
    }

    memset(&entry, 0, sizeof(entry));
    entry.id = argv[2];
    entry.name = entry.id;
    entry.command = argv[3];

    i = 4;
    while (i < argc) {
        if (strcmp(argv[i], "--name") == 0) {
            if (i + 1 >= argc) return 1;
            entry.name = argv[i + 1];
            i += 2;
            continue;
        }
        if (strcmp(argv[i], "--description") == 0) {
            if (i + 1 >= argc) return 1;
            entry.description = argv[i + 1];
            i += 2;
            continue;
        }
        if (strcmp(argv[i], "--icon") == 0) {
            if (i + 1 >= argc) return 1;
            entry.icon = argv[i + 1];
            i += 2;
            continue;
        }
        if (strcmp(argv[i], "--category") == 0) {
            if (i + 1 >= argc) return 1;
            entry.category = argv[i + 1];
            i += 2;
            continue;
        }

        fprintf(stderr, "menu: unknown add option: %s\n", argv[i]);
        return 1;
    }

    if (kc_menu_add(&entry) != KC_MENU_OK) {
        fprintf(stderr, "menu: failed to add entry: %s\n", entry.id);
        return 1;
    }
    return 0;
}

/**
 * Delete one application menu entry.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process status.
 */
static int kc_cli_delete(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "menu: delete requires <id>\n");
        return 1;
    }
    if (kc_menu_delete(argv[2]) != KC_MENU_OK) {
        fprintf(stderr, "menu: failed to delete entry: %s\n", argv[2]);
        return 1;
    }
    return 0;
}

/**
 * Execute the command-line interface.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process status.
 */
int main(int argc, char **argv) {
    if (argc == 2 &&
            (strcmp(argv[1], "-h") == 0 ||
            strcmp(argv[1], "--help") == 0)) {
        kc_cli_help();
        return 0;
    }
    if (argc == 2 &&
            (strcmp(argv[1], "-v") == 0 ||
            strcmp(argv[1], "--version") == 0)) {
        kc_cli_version();
        return 0;
    }
    if (argc < 2) {
        kc_cli_help();
        return 1;
    }
    if (strcmp(argv[1], "add") == 0) return kc_cli_add(argc, argv);
    if (strcmp(argv[1], "delete") == 0) return kc_cli_delete(argc, argv);

    fprintf(stderr, "menu: unknown command: %s\n", argv[1]);
    return 1;
}
