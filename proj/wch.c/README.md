# wch

`wch` creates, lists, replaces, and removes named resident filesystem watchers.
Each watcher runs a configured command when filesystem events occur.

## Quick start

Register a watcher:

```bash
wch source ./src ./handler.sh
```

Register it recursively:

```bash
wch source ./src --recursive ./handler.sh
```

List watchers:

```bash
wch -l
```

Remove the watcher:

```bash
wch source -d
```

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

Registration returns after the resident watcher has been created. The watcher
does not remain attached to the terminal and does not stream events to stdout.

`--recursive` is an option rather than a positional argument and may appear
before or after command arguments.

Listing uses this tab-separated schema:

```text
<id><TAB><path><TAB><cmd>
```

On Windows, the build packages `libwch.dll` beside `wch.exe`.

## Build

The Makefile requires `KCLIB_DIR` to point to the checkout that contains the
matching prebuilt library and public header.

```bash
export KCLIB_DIR=path/to/kclib
make
make test
```

Build every configured target with:

```bash
make all
```

`make all` requires the toolchains for the targets being built. Run the x86_64
Windows CLI tests through Wine with:

```bash
make test wine
```

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
