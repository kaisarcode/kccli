/**
 * wch.c - Resident file watcher command-line interface.
 * Summary: Manages libwch watchers and streams filesystem events.
 *
 * Author:  KaisarCode
 * Website: https://kaisarcode.com
 * License: GNU General Public License v3.0
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "libwch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/**
 * Reports whether one watcher name is valid for the CLI.
 * @param name Watcher name.
 * @return Nonzero when valid.
 */
static int wch_cli_valid_name(const char *name)
{
    const unsigned char *p;

    if (!name || !name[0]) return 0;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return 0;
    for (p = (const unsigned char *)name; *p; p++) {
        if ((*p >= 'a' && *p <= 'z') ||
            (*p >= 'A' && *p <= 'Z') ||
            (*p >= '0' && *p <= '9') ||
            *p == '.' || *p == '_' || *p == '-') {
            continue;
        }
        return 0;
    }
    return 1;
}

/**
 * Prints public CLI usage.
 * @param name Program executable name.
 * @return None.
 */
static void wch_cli_usage(const char *name)
{
    printf("Usage: %s <id> [path] [options]\n", name);
    printf("\n");
    printf("Commands:\n");
    printf("  %s <id> <path>       Create or replace and stream a watcher\n", name);
    printf("  %s <id>              Stream an existing watcher\n", name);
    printf("  %s -l [id]           List watchers\n", name);
    printf("  %s <id> -d           Delete a watcher\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  -r, --recursive      Watch directories recursively\n");
    printf("  -l, --list           List watchers\n");
    printf("  -d, --delete         Delete a watcher\n");
    printf("  -h, --help           Show this help\n");
    printf("  -v, --version        Show version\n");
}

/**
 * Prints the linked libwch build version.
 * @return None.
 */
static void wch_cli_version(void)
{
    printf("%llu\n", (unsigned long long)kc_wch_version());
}

/**
 * Prints one registered watcher as a tab-separated row.
 * @param entry Watcher entry.
 * @return None.
 */
static void wch_cli_print_entry(const kc_wch_entry_t *entry)
{
    if (!entry || !entry->name) return;
    printf(
        "%s\t%s\t%d\t%d\n",
        entry->name,
        entry->path ? entry->path : "",
        entry->recursive ? 1 : 0,
        entry->running ? 1 : 0
    );
}

/**
 * Lists all watchers or one named watcher.
 * @param filter Optional watcher name.
 * @return Process exit code.
 */
static int wch_cli_list(const char *filter)
{
    kc_wch_entry_t *entries = NULL;
    size_t count = 0;
    size_t i;

    if (kc_wch_list(&entries, &count) != KC_WCH_OK) return 1;
    for (i = 0; i < count; i++) {
        if (filter && strcmp(entries[i].name, filter) != 0) continue;
        wch_cli_print_entry(&entries[i]);
    }
    kc_wch_free(entries);
    return 0;
}

/**
 * Prints one watcher event to standard output.
 * @param event Event label.
 * @param path Changed filesystem path.
 * @return None.
 */
static void wch_cli_emit(const char *event, const char *path)
{
    printf("%s\t%s\n", event, path ? path : "");
    fflush(stdout);
}

/**
 * Handles add events from libwch.
 * @param path Changed filesystem path.
 * @param userdata Unused caller data.
 * @return None.
 */
static void wch_cli_on_add(const char *path, void *userdata)
{
    (void)userdata;
    wch_cli_emit("add", path);
}

/**
 * Handles update events from libwch.
 * @param path Changed filesystem path.
 * @param userdata Unused caller data.
 * @return None.
 */
static void wch_cli_on_upd(const char *path, void *userdata)
{
    (void)userdata;
    wch_cli_emit("upd", path);
}

/**
 * Handles delete events from libwch.
 * @param path Changed filesystem path.
 * @param userdata Unused caller data.
 * @return None.
 */
static void wch_cli_on_del(const char *path, void *userdata)
{
    (void)userdata;
    wch_cli_emit("del", path);
}

/**
 * Sleeps while temporary subscriptions receive watcher events.
 * @return None.
 */
static void wch_cli_wait(void)
{
#ifdef _WIN32
    Sleep(1000);
#else
    struct timespec delay;

    delay.tv_sec = 1;
    delay.tv_nsec = 0;
    nanosleep(&delay, NULL);
#endif
}

/**
 * Streams all events from one registered watcher.
 * @param name Watcher name.
 * @return Process exit code.
 */
static int wch_cli_stream(const char *name)
{
    kc_wch_t *watcher = NULL;
    int status;

    status = kc_wch_open(&watcher, name);
    if (status != KC_WCH_OK || watcher == NULL) return 1;
    if (kc_wch_get_path(watcher) == NULL) {
        kc_wch_close(watcher);
        return 1;
    }
    if (kc_wch_on(watcher, "add", wch_cli_on_add, NULL) != KC_WCH_OK ||
        kc_wch_on(watcher, "upd", wch_cli_on_upd, NULL) != KC_WCH_OK ||
        kc_wch_on(watcher, "del", wch_cli_on_del, NULL) != KC_WCH_OK) {
        kc_wch_close(watcher);
        return 1;
    }
    for (;;) wch_cli_wait();
}

/**
 * Returns the private neutral command used by the CLI watcher.
 * @return Platform-specific neutral command.
 */
static const char *wch_cli_neutral_command(void)
{
#ifdef _WIN32
    return "rem";
#else
    return ":";
#endif
}

/**
 * Creates or replaces one resident watcher.
 * @param name Watcher name.
 * @param path Watched path.
 * @param recursive Nonzero enables recursive watching.
 * @return Process exit code.
 */
static int wch_cli_create(
    const char *name,
    const char *path,
    int recursive
)
{
    kc_wch_options_t options;

    memset(&options, 0, sizeof(options));
    options.path = path;
    options.cmd = wch_cli_neutral_command();
    options.recursive = recursive;
    return kc_wch_create(name, &options) == KC_WCH_OK ? 0 : 1;
}

/**
 * Runs the wch CLI.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
int main(int argc, char **argv)
{
    const char *name = NULL;
    const char *path = NULL;
    int recursive = 0;
    int list = 0;
    int remove = 0;
    int i;

    if (argc == 2 &&
        (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        wch_cli_usage(argv[0]);
        return 0;
    }
    if (argc == 2 &&
        (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)) {
        wch_cli_version();
        return 0;
    }

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 ||
            strcmp(argv[i], "--recursive") == 0) {
            recursive = 1;
        } else if (strcmp(argv[i], "-l") == 0 ||
            strcmp(argv[i], "--list") == 0) {
            list = 1;
        } else if (strcmp(argv[i], "-d") == 0 ||
            strcmp(argv[i], "--delete") == 0) {
            remove = 1;
        } else if (argv[i][0] == '-') {
            return 1;
        } else if (!name) {
            name = argv[i];
        } else if (!path) {
            path = argv[i];
        } else {
            return 1;
        }
    }

    if (list) {
        if (remove || recursive || path) return 1;
        if (name && !wch_cli_valid_name(name)) return 1;
        return wch_cli_list(name);
    }
    if (!name || !wch_cli_valid_name(name)) return 1;
    if (remove) {
        if (path || recursive) return 1;
        return kc_wch_delete(name) == KC_WCH_OK ? 0 : 1;
    }
    if (!path) {
        if (recursive) return 1;
        return wch_cli_stream(name);
    }
    if (wch_cli_create(name, path, recursive) != 0) return 1;
    return wch_cli_stream(name);
}
