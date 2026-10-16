# punch

`punch` sends or receives one UDP datagram over a direct peer-to-peer path.
Both peers run the same CLI; endpoint discovery and signaling are intentionally
outside its scope.

## Quick start

Test a direct exchange on loopback using two terminals.

Terminal A:

```bash
punch receive 127.0.0.1:5000 -p 5001 > message.bin
```

Terminal B:

```bash
printf 'hello' | punch send 127.0.0.1:5001 -p 5000
```

## Usage

```text
Usage: punch <send|receive> <peer> [peer...] [options]

Options:
  --host <host>           Local bind host
  -p, --port <port>       Local UDP port (default: 0)
  -t, --timeout <ms>      Connection timeout in ms (default: library)
  -i, --input <file>      Read send datagram from file
  --receive-timeout <ms>  Receive timeout in ms (default: 0, infinite)
  -h, --help              Show this help
  -v, --version           Show version
```

Peer endpoints are positional and may be repeated. IPv6 literals use bracket
form so the address and UDP port remain unambiguous.

`send` reads one binary datagram from redirected stdin first, otherwise from
`--input`. `receive` writes exactly one received datagram to standard output.

See [Direct UDP operation](doc/direct-udp.md) for endpoint exchange, concurrent
hole punching, multiple peers, timeouts, and the protocol boundaries that
remain outside this CLI.

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
