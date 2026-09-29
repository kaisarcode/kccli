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

## Beta Notice

This is a beta project tested only on Debian x86_64. It was created out of a
personal need for these libraries, but no guarantees are provided regarding its
stability or future support. You are free to test it, use it, and modify it as
you please.

If you'd like to reach out, you can send an email to kaisar@kaisarcode.com.
Please note that I do not accept pull requests; the goal is to avoid long-term
dependency on platforms like GitHub, and I do not maintain fixed infrastructure
to guarantee long-term stability for these projects.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
