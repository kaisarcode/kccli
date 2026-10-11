# mmap

`mmap` stores persistent binary values in backing files.

It operates on one backing file at a time. Values are binary-safe: `get` writes
stored bytes directly to standard output, and `set` reads bytes from a file or
standard input.

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

Read a stored value:

```bash
mmap get state.bin
```

Write bytes from a file:

```bash
mmap set state.bin -i payload.bin
```

Write bytes from standard input:

```bash
printf 'hello' | mmap set state.bin
```

An empty input is a real zero-byte value and creates or truncates the backing
file to zero bytes. It is distinct from a missing value.

Delete the value and backing file:

```bash
mmap del state.bin
```

A missing value causes `get` to return a nonzero exit status. A stored zero-byte
value causes `get` to succeed while writing zero bytes.

## Build

```bash
make
```

Build all configured desktop targets:

```bash
make all
```

## Tests

```bash
make test
```

Windows tests can be run through Wine:

```bash
make test wine
```

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
