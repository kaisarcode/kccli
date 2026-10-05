# wch

`wch` is a command-line frontend for `libwch.c` resident filesystem watchers.

It provides a terminal-friendly interface without changing the public `libwch`
API or its behavior.

## Usage

```text
Usage: wch <id> [path] [options]

Commands:
  wch <id> <path>       Create or replace and stream a watcher
  wch <id>              Stream an existing watcher
  wch -l [id]           List watchers
  wch <id> -d           Delete a watcher

Options:
  -r, --recursive      Watch directories recursively
  -l, --list           List watchers
  -d, --delete         Delete a watcher
  -h, --help           Show this help
  -v, --version        Show version
```

## Watch a path

Create or replace a named watcher and immediately stream its events:

```bash
wch source ./src
```

Watch directories recursively:

```bash
wch source ./src -r
```

The CLI keeps running and writes each event to standard output as one
headerless tab-separated row:

```text
<event><TAB><path>
```

For example:

```text
add	/home/user/project/new.txt
upd	/home/user/project/main.c
del	/home/user/project/old.txt
```

The event name is one of `add`, `upd`, or `del`.

The command required internally by `libwch.c` is an implementation detail of
this CLI and is not exposed as part of the `wch` interface.

## Stream an existing watcher

Open an existing watcher by name and stream its events without replacing its
configuration:

```bash
wch source
```

This makes the output easy to compose with another command:

```bash
wch source | ./consumer
```

The consumer can parse the event name and path directly from standard input.

## List watchers

List every registered watcher:

```bash
wch -l
```

List one watcher:

```bash
wch -l source
```

Each row is headerless TSV with these fields:

```text
<id><TAB><path><TAB><recursive><TAB><running>
```

The final two fields are `0` or `1`. The private command used internally by the
CLI is intentionally not exposed.

## Delete a watcher

```bash
wch source -d
```

Deleting a watcher stops and removes the resident watcher through `libwch.c`.

## Build

The CLI links against the precompiled `libwch.c` artifacts in `kclib/dist`.

```bash
make
```

Build all configured desktop targets:

```bash
make all
```

Linux and macOS link the static `libwch` artifact. Windows uses the `libwch.dll`
import library and packages the DLL beside `wch.exe`, preserving the resident
watcher implementation provided by `libwch.c`.

## Tests

```bash
make test
```

Windows tests can be run through Wine:

```bash
make test wine
```

## License

GNU General Public License version 3 or later.
