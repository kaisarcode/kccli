# hpm

`hpm` sends or receives one UDP datagram over a direct peer-to-peer path backed by `libhpm.c`.

Both peers run the same CLI. There are no publisher/consumer roles. Each side supplies the remote endpoint information it already knows and a local bind preference; HPM performs the direct connectivity attempt before the datagram operation.

## Usage

```text
Usage: hpm <command> <peer> [peer...] [options]

Commands:
  send                       Send one UDP datagram
  receive                    Receive one UDP datagram

Endpoint format:
  host:port                  IPv4 or hostname endpoint
  [ipv6]:port                IPv6 endpoint

Connection options:
  --host <host>              Local bind host
  -p, --port <port>          Local UDP port (default: 0)
  -t, --timeout <ms>         Connection timeout in ms (default: library)

send options:
  -i, --input <file>         Read datagram from file

receive options:
  --receive-timeout <ms>     Receive timeout in ms (default: 0, infinite)

Common options:
  -h, --help                 Show this help
  -v, --version              Show version
```

Peer endpoints are positional and may be repeated. HPM tries the supplied endpoints and uses a viable direct path. IPv6 literals use bracket form so the address and UDP port remain unambiguous.

`send` reads exactly one datagram. Redirected standard input is used first; otherwise `-i`/`--input` supplies the datagram from a file. Binary data and empty datagrams are preserved.

`receive` writes exactly one received datagram to standard output. Output is binary and contains no status or endpoint decoration. `--receive-timeout 0` waits indefinitely after the direct path has been established.

For example, on two terminals using loopback endpoints:

Terminal A:

```bash
hpm receive 127.0.0.1:5000 -p 5001 > message.bin
```

Terminal B:

```bash
printf 'hello' | hpm send 127.0.0.1:5001 -p 5000
```

Multiple known endpoints can be supplied for the same peer:

```bash
cat payload.bin | hpm send \
  203.0.113.20:5000 \
  192.168.1.20:5000 \
  -p 5000
```

The CLI does not discover or exchange endpoints. It does not perform STUN, rendezvous, index lookup, relay/TURN, identity, authentication, encryption, or reliable delivery. Endpoint exchange and any higher-level session policy remain outside `hpm`.

## Build

```bash
make
```

Build all configured targets:

```bash
make all
```

The build links the matching precompiled static `libhpm.c` artifact from `kclib/dist`.

## Tests

```bash
make test
```

The black-box tests cover argument validation and a real loopback exchange between two `hpm` processes, including multiple peer endpoints and binary datagrams.

Windows tests can be run through Wine:

```bash
make test wine
```

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
