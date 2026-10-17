# rstrm

`rstrm` relays a reliable ordered full-duplex byte stream between standard
streams and an explicit UDP peer. The CLI owns the UDP datagram path while
`librstrm` owns reliability, ordering, retransmission, flow control, and stream
half-close behavior.

Both endpoints identify each other directly. The command does not perform peer
discovery, rendezvous, authentication, encryption, or multiplexing.

## Quick start

Run one endpoint with local UDP port 9000 and peer UDP port 9001:

```bash
rstrm 10.0.0.2 9001 -p 9000 < a.in > a.out
```

Run the other endpoint with the addresses reversed:

```bash
rstrm 10.0.0.1 9000 -p 9001 < b.in > b.out
```

Bytes from each input file are delivered in order to the opposite standard
output. Both directions can carry data at the same time.

## Usage

```text
rstrm <peer-host> <peer-port> [options]
```

Options:

- `--host <host>` selects the local UDP bind host. The default is the wildcard
    address for the selected peer address family.
- `-p, --port <port>` selects the local UDP port. The default is `0`, which lets
    the operating system choose an ephemeral port.
- `--mtu <bytes>` selects the maximum datagram size passed to `librstrm`.
    Omitting the option or using `0` selects the library default. Other values
    are validated by the public library contract.
- `-h, --help` shows command help.
- `-v, --version` prints the linked `librstrm` build version.

For a normal two-endpoint relay, use explicit nonzero local ports so each peer
can name the other endpoint before any stream data is sent.

## Standard streams

Standard input and output are binary-safe. Bytes read from stdin are queued as
stream bytes, and received stream bytes are written unchanged to stdout.
Diagnostics are written only to stderr.

When stdin reaches EOF, `rstrm` gracefully closes only its local write side and
continues receiving peer data. Receiving peer EOF does not disable local sends.
The command exits normally after local stdin has reached EOF and peer EOF has
also been received. Interrupting the command closes the local stream without
waiting for the peer.

If the stream reports send backpressure, stdin consumption pauses until send
capacity returns. Incoming UDP datagrams remain active while local sending is
paused.

## UDP path

Each endpoint sends complete `librstrm` datagrams to the configured peer host
and port. Each received UDP datagram is passed to the library as one complete
input datagram only when its source matches that configured peer endpoint.

The UDP path is exclusive to one `rstrm` session. The CLI does not add a
handshake or session protocol above the reliable stream.

## Build

The Makefile requires `KCLIB_DIR` to point to a checkout containing the matching
prebuilt `librstrm` artifacts and public header.

```bash
export KCLIB_DIR=path/to/kclib
make
make test
```

Build every configured target with:

```bash
make all
```

Run the x86_64 Windows CLI tests through Wine with:

```bash
make test wine
```

Configured CLI build targets cover Linux, Windows, macOS, and Android on the
architectures provided by the Makefile and matching `librstrm` artifacts.

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
