# menu

`menu` creates, replaces, and removes native application-menu entries.

## Quick start

Register an application:

```bash
menu myapp ./myapp --name "My App"
```

Remove it:

```bash
menu myapp -d
```

## Usage

```text
Usage: menu <id> [command] [options]

Commands:
  menu <id> <cmd>       Register or replace a menu entry
  menu <id> -d          Remove a menu entry

Options:
  --name <name>         Application name (defaults to id)
  --description <text>  Application description
  --icon <path>         Application icon path
  --category <name>     Application menu category
  -d, --delete          Remove a menu entry
  -h, --help            Show this help
  -v, --version         Show version
```

The id and command are required when registering an entry. If `--name` is
omitted, the id is used as the display name.

Linux uses the current user's XDG application menu. Windows uses the current
user's Start Menu Programs folder.

Deleting a missing entry is a successful no-op.

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
