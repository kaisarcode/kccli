# init.c - Persistent Startup Registration CLI

`init` is the command-line interface for `libinit.c`.

The project does not compile libinit source. It links against the precompiled
static library and public header for each target:

```text
kclib/dist/libinit.c/<arch>/<platform>/libinit.h
kclib/dist/libinit.c/<arch>/<platform>/libinit.a
```

The resulting executable does not require the libinit shared library at
runtime.

## Build

Build the native host target:

```sh
make
```

Build all configured Linux, Windows, and macOS targets:

```sh
make all
```

By default, `kclib` is expected as a sibling checkout of `kccli`. Override
the location when necessary:

```sh
make KCLIB_DIST_DIR=/path/to/kclib/dist
```

## Tests

Run the native CLI contract:

```sh
make test
```

Run the x86_64 Windows CLI contract through Wine:

```sh
make test wine
```

Tests exercise the final executable statically linked to libinit and use an isolated
`KC_INIT_DIR` namespace for non-destructive listing checks.

## Usage

```text
Usage: init <name> [command...]

Commands:
  init <name> <cmd>    Register or replace a startup entry
  init -l [name]       List registrations
  init <name> -l       List one registration
  init -d <name>       Remove a registration
  init <name> -d       Remove a registration

Options:
  -l, --list           List registrations
  -d, --delete         Remove a registration
  --dir <path>         Override metadata directory
  -h, --help           Show this help
  -v, --version        Show version
```

Linux, macOS, and Windows behavior follows the linked `libinit.c`
implementation.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
