# ppn

`ppn` displays one native operating-system notification with a title and
message.

## Quick start

```bash
ppn "Download complete" "package.tar.gz is ready"
```

The operating system controls presentation, duration, grouping, and visibility
according to the user's notification settings.

## Usage

```text
Usage: ppn <title> <message>

Options:
  -h, --help     Show this help
  -v, --version  Show version
```

Both arguments are required and must be non-empty. Quote arguments that contain
spaces.

## Platforms

Configured targets are:

```text
x86_64/linux
x86_64/windows
x86_64/macos
aarch64/macos
```

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

The CLI tests validate its argument contract. Native notification integration
belongs to the linked platform library and may require a graphical session at
runtime.

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
