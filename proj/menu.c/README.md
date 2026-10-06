# menu.c - Native Application Menu CLI

`menu` manages native application-menu entries from the command line.

It adds application launchers and deletes them by identifier. Linux uses the
current user's XDG application menu and Windows uses the current user's Start
Menu Programs folder.

## Build

Build the native host target:

```sh
make
```

Build all configured Linux and Windows targets:

```sh
make all
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

`id` and `command` are required when registering an entry. `--name` is optional;
when omitted, the entry name is the same as `id`.

Deleting a missing entry is a successful no-op.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
