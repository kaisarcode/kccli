# nets

`nets` is a binary relay for one persistent outbound TCP or UDP endpoint. It
sends standard input through the endpoint and writes received network bytes
unchanged to standard output.

The CLI handles raw transport only. It does not implement TLS, HTTP, or another
application protocol.

## Quick start

Connect to a local TCP service and exchange raw bytes:

```bash
printf 'hello' | nets 127.0.0.1 9000
```

Use UDP instead of the default TCP transport:

```bash
nets 127.0.0.1 9000 --transport udp
```

## Usage

```text
nets <host> <port> [options]
```

Options:

- `-t, --transport <tcp|udp>` selects the transport. The default is `tcp`.
- `-h, --help` shows command help.
- `-v, --version` prints the linked library build version.

TLS is intentionally not an option. Secure or higher-level protocols belong to
separate components composed outside this CLI.

## Standard streams

Standard input is treated as binary data. Each chunk read from stdin is sent
through the same persistent endpoint. Received bytes are written unchanged to
standard output. Diagnostics are written to standard error.

For TCP, reaching EOF on standard input stops local writes but leaves the
connection open for incoming data. The command exits successfully when the
remote peer disconnects. Interrupt the command to close the local endpoint.

For UDP, reaching EOF also stops local sends, but UDP has no disconnect event.
The endpoint therefore remains open for incoming datagrams until the command is
interrupted or a network error occurs.

## Build

The Makefile requires `KCLIB_DIR` to point to the checkout containing the
matching prebuilt `libnets` artifacts and public header.

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

This project is distributed under the **GNU Lesser General Public License
version 3 (LGPLv3)**.
