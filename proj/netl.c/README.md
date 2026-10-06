# netl.c - Incoming Network Listener CLI

`netl` listens for incoming TCP or UDP traffic and dispatches it to a local
command.

The CLI intentionally exposes a small terminal-oriented surface over
`libnetl.c`. It does not project the library callbacks or peer-management API.

## Usage

```text
Usage: netl <port> <command...> [options]

Options:
  -t, --transport <type>  tcp or udp (default: tcp)
  -h, --help              Show this help
  -v, --version           Show version
```

TCP is used when `--transport` is omitted.

Options are not positional. `-t` / `--transport` may appear anywhere in the
command line and its value is the following argument.

Examples:

```bash
netl 8080 ./handler.sh
netl 8080 -t tcp ./handler.sh
netl 5353 ./handler.sh --transport udp
```

If the configured command itself contains arguments beginning with `-`, pass
the command as one quoted argument so those tokens remain part of the command:

```bash
netl 8080 "./handler.sh --production"
```

## Command behavior

For TCP, one command process is started for each accepted connection. Incoming
connection bytes are written to the command's standard input and command
standard output is sent back to that same peer. The process remains associated
with the connection until either side closes.

For UDP, one command process is started for each datagram and the datagram is
written to its standard input. Command output is not automatically sent back to
the UDP peer.

`netl` remains active until interrupted or terminated. It does not create a
persistent registration or background service.

## Build

The CLI links against the precompiled `libnetl.c` artifacts in `kclib/dist`.

```bash
make
```

Build all configured desktop targets:

```bash
make all
```

## Tests

Run the native CLI contract and TCP dispatch test:

```bash
make test
```

Run the x86_64 Windows contract through Wine:

```bash
make test wine
```

## License

GNU General Public License version 3 or later.
