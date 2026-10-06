# wch

`wch` is a command-line frontend for `libwch.c` resident filesystem watchers.

It provides a terminal-friendly interface without changing the public `libwch`
API or its behavior.

## Usage

```text
Usage: wch <id> <path> <command...>

Commands:
  wch <id> <path> <command...>  Register or replace a watcher
  wch -l [id]                    List watchers
  wch <id> -d                    Delete a watcher

Options:
  -r, --recursive    Watch directories recursively
  -l, --list         List watchers
  -d, --delete       Delete a watcher
  -h, --help         Show this help
  -v, --version      Show version
```

## Register a watcher

Register or replace a resident watcher:

```bash
wch source ./src ./handler.sh
```

The command returns the terminal immediately after the watcher is registered or
replaced. `wch` does not remain attached to the terminal and does not stream
watch events to standard output.

The configured command is executed by `libwch.c` when filesystem events occur.
The command may contain multiple positional arguments; `wch` stores them as one
command string.

For example:

```bash
wch source ./src ./handler.sh rebuild
```

Watch directories recursively with either option form:

```bash
wch source ./src -r ./handler.sh
```

```bash
wch source ./src ./handler.sh --recursive
```

Options are not positional, so `-r` or `--recursive` may appear anywhere in the
command line without changing the meaning of `id`, `path`, or `command...`.

## List watchers

List every registered watcher:

```bash
wch -l
```

List one watcher:

```bash
wch -l source
```

Both forms use the same fixed, tab-separated output schema:

```text
<id><TAB><path><TAB><cmd>
```

No runtime state or additional configuration fields are printed.

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
