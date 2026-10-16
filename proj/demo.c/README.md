# demo

`demo` creates a greeting for one name.

## Quick start

```bash
demo John
```

```text
Hello John!
```

Quote names that contain spaces:

```bash
demo "John Doe"
```

## Usage

```text
Usage: demo <name> [options]

Options:
  -h, --help     Show this help
  -v, --version  Show version
```

Exactly one name is accepted.

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
