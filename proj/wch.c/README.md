# wch

`wch` is a command-line frontend for `libwch.c` resident filesystem watchers.

It provides a terminal-friendly interface without changing the public `libwch`
API or its behavior.

## Usage

```text
Usage: wch <id> [path] [options]

Commands:
  wch <id> <path> -c <cmd>  Create or replace a watcher
  wch <id>                  Stream watcher events
  wch -l [id]               List watchers
  wch <id> -d               Delete a watcher

Options:
  -c, --cmd <command>   Persistent event command
  -r, --recursive       Watch directories recursively
  -l, --list            List watchers
  -d, --delete          Delete a watcher
  -h, --help            Show this help
  -v, --version         Show version
```

## Create a watcher

Creating a resident watcher requires a persistent command because `libwch`
requires `cmd` when creating one.

```bash
wch source ./src -c ./handler.sh
```

Watch directories recursively:

```bash
wch source ./src -r -c ./handler.sh
```

The persistent command receives the event and changed path according to the
existing `libwch.c` command contract.

Commands containing spaces must be quoted as one CLI argument:

```bash
wch source ./src -c "./handler.sh --reload"
```

## Stream events

Open an existing watcher by name without a path to stream all of its events:

```bash
wch source
```

Each event is printed as one headerless tab-separated row:

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

This output is intended to compose naturally with other commands:

```bash
wch source | ./consumer
```

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

On Windows the CLI uses the `libwch.dll` import library and packages the DLL
beside `wch.exe`, preserving the resident watcher behavior implemented by
`libwch.c`.

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
