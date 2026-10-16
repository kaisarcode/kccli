# flow

`flow` runs a `.flow` document, applies temporary ordered overrides, and writes
successful output to standard output.

## Quick start

```bash
flow example.flow
```

Run one entry and provide binary input from a file:

```bash
flow example.flow --entry build --input payload.bin
```

## Usage

```text
Usage: flow <file.flow> [options]

Options:
  -e, --entry <node>       Start from one entry node
  -i, --input <path>       Input file; use '-' for stdin
  -s, --set <key> <value>  Set an override; repeatable
  -u, --unset <key>        Remove an override; repeatable
  -h, --help               Show this help
  -v, --version            Show version
```

## Execution

`--set` and `--unset` may be repeated. Their order is preserved and the changes
apply only to the current invocation.

If `--entry` is omitted, the declared flow entries are used. If `--input` is
omitted, no input buffer is supplied. Use `--input -` to read standard input.

Successful output is written unchanged to stdout and diagnostics go to stderr.

`Ctrl+C` and termination signals request cooperative cancellation. The active
step finishes before the run stops, and a cooperatively stopped run exits with
status `130`.

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
