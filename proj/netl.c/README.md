# netl

`netl` creates, lists, replaces, and removes named resident TCP or UDP listeners
that dispatch incoming traffic to local commands.

## Quick start

Register a TCP listener:

```bash
netl web 8080 ./handler.sh
```

List listeners:

```bash
netl -l
```

Remove the listener:

```bash
netl web -d
```

## Usage

```text
Usage: netl <id> <port> <command...>

Commands:
  netl <id> <port> <cmd>  Register or replace a listener
  netl -l [id]             List listeners
  netl <id> -d             Remove a listener

Options:
  -t, --transport <type>   tcp or udp (default: tcp)
  -l, --list               List listeners
  -d, --delete             Remove a listener
  -h, --help               Show this help
  -v, --version            Show version
```

Registration returns after the resident listener has been created. Listener ids
may contain letters, digits, dots, underscores, and hyphens.

Use UDP explicitly with `--transport udp`.

## Runtime state

`KC_NETL_DIR` overrides the directory used for resident listener state. Without
it, `netl` selects a per-user runtime directory appropriate for the platform.

Listing prints one tab-separated row per listener:

```text
<id><TAB><port><TAB><transport><TAB><cmd>
```

See [Listener behavior](doc/listener-behavior.md) for TCP and UDP command
dispatch semantics and lifecycle details.

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
