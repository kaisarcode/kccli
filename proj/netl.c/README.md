# netl.c - Resident Network Listener CLI

`netl` manages named resident TCP or UDP listeners.

A listener is registered once, remains active after the command returns, and
dispatches incoming traffic to the configured local command.

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

Arguments beginning with `-` or `--` are options, not positional arguments.
Options that take a value consume the following argument.

## Register a listener

TCP is the default transport:

```bash
netl web 8080 ./handler.sh
```

Use UDP explicitly:

```bash
netl dns 5353 -t udp ./handler.sh
```

Registration creates or replaces the named resident listener and returns the
terminal immediately.

For TCP, one command process is associated with each connection. Bytes received
from the peer are written to the command's standard input, and command standard
output is sent back to the same peer.

For UDP, each datagram starts one command and the datagram is written to that
command's standard input.

## List listeners

List all registrations:

```bash
netl -l
```

List one registration:

```bash
netl -l web
```

Each row has a fixed machine-readable schema:

```text
<id><TAB><port><TAB><transport><TAB><cmd>
```

Example:

```text
web 8080    tcp ./handler.sh
dns 5353    udp ./dns-handler.sh
```

## Remove a listener

```bash
netl web -d
```

This stops the resident listener process and removes its registration.

## Runtime state

`KC_NETL_DIR` can override the runtime state directory when needed.

## Build

```bash
make
```

Build all configured desktop targets:

```bash
make all
```

## Tests

```bash
make test
```

Run the Windows tests through Wine:

```bash
make test wine
```

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-127x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
