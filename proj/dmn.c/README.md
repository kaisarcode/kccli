# dmn

`dmn` creates, lists, uses, and removes named resident local daemons. A
registered daemon remains available after the command that created it exits.

## Quick start

Register or replace a daemon:

```bash
dmn worker cat
```

Exchange one request with the daemon:

```bash
printf 'hello' | dmn worker
```

List registrations:

```bash
dmn -l
```

Remove the daemon:

```bash
dmn worker -d
```

## Usage

```text
Usage: dmn <id> [command...]

Commands:
  dmn <id> <cmd>  Create or update a daemon
  dmn -l [id]      List daemons
  dmn <id> -d      Delete a daemon
  dmn <id>         Send stdin to a daemon

Options:
  -l, --list       List daemons
  -d, --delete     Delete a daemon
  -h, --help       Show this help
  -v, --version    Show version
```

## Lifecycle and data exchange

Registering an existing id replaces its configured command. Creation and update
return after the resident daemon is registered.

`dmn <id>` reads one request from standard input, sends it to the existing
daemon, and writes the response to standard output. Request framing is handled
internally.

`dmn -l` and `dmn -l <id>` print the same tab-separated schema:

```text
<id><TAB><cmd>
```

Deleting an id stops and removes that daemon.

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
