# mmap

`mmap` reads, replaces, and deletes one persistent binary value stored in a
backing file.

## Quick start

Store bytes from standard input:

```bash
printf 'hello' | mmap set state.bin
```

Read them back:

```bash
mmap get state.bin
```

Remove the value:

```bash
mmap del state.bin
```

## Usage

```text
Usage: mmap <command> <path> [options]

Commands:
  get <path>           Write the stored bytes to stdout
  set <path>           Replace and persist the stored bytes
  del <path>           Delete the stored value and backing file

set options:
  -i, --input <path>   Input file; use '-' for stdin (default: -)

Common options:
  -h, --help           Show this help
  -v, --version        Show version
```

## Binary values

`get` writes stored bytes directly to stdout. `set` is binary-safe and reads
stdin by default or the file selected with `--input`.

An empty input is a real zero-byte value and is different from a missing value.
Reading a missing value fails; reading a stored zero-byte value succeeds and
writes no bytes.

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
