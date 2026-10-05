# wch

`wch` is a command-line frontend for `libwch.c` resident filesystem watchers.

It provides a terminal-friendly interface without changing the public `libwch`
API or its behavior.

## Usage

```text
Usage: wch <id> <path> -c <cmd> [options]

Commands:
  wch <id> <path> -c <cmd>  Create or replace a watcher
  wch -l [id]               List watchers
  wch <id> -d               Delete a watcher

Options:
  -c, --cmd <command>   Event command
  -r, --recursive       Watch directories recursively
  -l, --list            List watchers
  -d, --delete          Delete a watcher
  -h, --help            Show this help
  -v, --version         Show version
```

## Create a watcher

Define the watcher in one command:

```bash
wch source ./src -c ./handler.sh
```

The watcher is resident. The CLI returns immediately after creating or replacing
the registration; it does not keep the terminal open.

Watch directories recursively:

```bash
wch source ./src -r -c ./handler.sh
```

Commands containing spaces must be quoted as one CLI argument:

```bash
wch source ./src -c "./handler.sh --reload"
```

The configured command is executed by `libwch.c` when an event occurs, using the
existing library command contract.

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
<id><TAB><path><TAB><cmd><TAB><recursive><TAB><running>
```

The final two fields are `0` or `1`.

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
