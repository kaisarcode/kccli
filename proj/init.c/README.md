# init.c - Persistent Startup Registration CLI

`init` manages persistent operating-system startup registrations from the
command line.

It creates or replaces startup commands by identifier, lists existing
registrations, and removes registrations. It does not start, stop, supervise, or
execute the registered commands in the current process.

## Build

Build the native host target:

```sh
make
```

Build all configured Linux, Windows, and macOS targets:

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
Usage: init <id> [command...]

Commands:
  init <id> <cmd>      Register or replace a startup entry
  init -l [id]         List registrations
  init <id> -l         List one registration
  init -d <id>         Remove a registration
  init <id> -d         Remove a registration

Options:
  -l, --list           List registrations
  -d, --delete         Remove a registration
  -h, --help           Show this help
  -v, --version        Show version
```

Linux, macOS, and Windows behavior follows the linked `libinit.c`
implementation.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
