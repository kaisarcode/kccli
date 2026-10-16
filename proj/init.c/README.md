# init

`init` creates, lists, replaces, and removes persistent operating-system startup
registrations. It does not execute or supervise the registered command in the
current process.

## Quick start

Register a command:

```bash
init my-service ./service
```

List registrations:

```bash
init -l
```

Remove it:

```bash
init my-service -d
```

## Usage

```text
Usage: init <id> [command...]

Commands:
  init <id> <cmd>      Register or replace a startup entry
  init -l [id]         List registrations
  init <id> -d         Remove a registration

Options:
  -l, --list           List registrations
  -d, --delete         Remove a registration
  -h, --help           Show this help
  -v, --version        Show version
```

## Listing

Both list forms use the same tab-separated schema:

```text
<id><TAB><user><TAB><cmd>
```

Registering an existing id replaces that registration.

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
