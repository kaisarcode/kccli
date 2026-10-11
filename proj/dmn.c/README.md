# dmn.c - Local Daemon Manager CLI

`dmn` manages named resident processes.

Creating a daemon returns immediately. The configured process remains resident
and can be addressed later by its id. Registering the same id again updates the
daemon configuration.

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

Run the native CLI tests:

```sh
make test
```

Run the x86_64 Windows CLI contract through Wine:

```sh
make test wine
```

## CLI

```text
dmn <id> [command...]
```

Commands:

```text
dmn <id> <cmd>  Create or update a daemon
dmn -l [id]      List daemons
dmn <id> -d      Delete a daemon
dmn <id>         Send stdin to a daemon
```

Options:

```text
-l, --list       List daemons
-d, --delete     Delete a daemon
-h, --help       Show help
-v, --version    Show version
```

Examples:

```bash
dmn worker ./worker.sh
dmn -l
dmn -l worker
printf 'request\n' | dmn worker
dmn worker -d
```

Command arguments are joined into one command string when the daemon is
registered. Tokens beginning with `-` or `--` are CLI options rather than
positional command arguments. Quote a complete shell command when it needs its
own option-like arguments.

## Listing

`dmn -l` and `dmn -l <id>` use the same fixed tab-separated schema:

```text
<id><TAB><cmd>
```

Runtime sockets, pipes, process ids, and other transport details are not part of
the CLI output.

## Data exchange

`dmn <id>` opens the registered daemon, reads one request from stdin until EOF,
and writes the response to stdout.

The framing used for request boundaries is handled internally. Users do not
need to append or strip framing bytes themselves.

## Lifecycle

A successful create or update leaves the daemon resident and returns control to
the terminal. Reusing the same id updates the existing daemon. `dmn <id> -d`
stops and removes it.

Normal daemon management should not require users to inspect process ids or use
platform process-management commands.

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-127x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
