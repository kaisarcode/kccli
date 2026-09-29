# kccli

`kccli` contains small native command-line tools built on top of kclib.

The current tool is `init`, the command-line interface for `libinit.c`.
`kccli` does not compile the library source itself. It links against the
precompiled static kclib artifact for each target:

```text
kclib/dist/libinit.c/<arch>/<platform>/libinit.h
kclib/dist/libinit.c/<arch>/<platform>/libinit.a
```

The resulting executable is self-contained with respect to libinit; it does
not require `libinit.so`, `libinit.dll`, or `libinit.dylib` at runtime.

## Build

By default the native host target is built:

```sh
make
```

Build every configured Linux, Windows, and macOS target:

```sh
make all
```

`kclib` is expected as a sibling checkout by default:

```text
work/
|-- kccli/
`-- kclib/
```

Use `KCLIB_DIST_DIR` to point at another generated kclib distribution:

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

The tests exercise the final statically linked executable and use an isolated
`KC_INIT_DIR` namespace for non-destructive listing checks.

## Distribution

Build all targets and package them by platform and architecture:

```sh
make dist
```

Packages are written under:

```text
dist/init/
```

Each package contains only the statically linked `init` executable plus the
project README and license.

## init

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

`init` delegates persistent startup registration to `libinit.c`. Linux, macOS,
and Windows behavior therefore follows the linked libinit implementation.
