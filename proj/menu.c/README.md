# menu.c - Native Application Menu CLI

`menu` manages native application-menu entries from the command line through
`libmenu.c`.

It adds application launchers and deletes them by identifier. Linux uses the
current user's XDG application menu and Windows uses the current user's Start
Menu Programs folder, following the linked `libmenu.c` implementation.

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
Usage: menu <command> [arguments]

Commands:
  menu add <id> <name> <command> [options]
      Add an application menu entry
  menu delete <id>
      Delete an application menu entry

Add options:
  --description <text>  Application description
  --icon <path>         Application icon path
  --category <name>     Application menu category

Options:
  -h, --help            Show this help
  -v, --version         Show version
```

`id`, `name`, and `command` are required by `libmenu.c`. Quote `name`,
`command`, or optional values when they contain spaces.

Deleting a missing entry is a successful no-op, matching the library contract.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
