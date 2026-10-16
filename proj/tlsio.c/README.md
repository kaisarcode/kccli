# tlsio

`tlsio` opens one authenticated TLS client or server session over TCP and relays
plaintext between standard input and standard output.

Standard input is sent as TLS application data. Authenticated plaintext received
from the peer is written unchanged to standard output. TCP is owned by the CLI;
TLS handshake, verification, encryption, decryption, and shutdown are provided
by the linked library.

## Quick start

Send an HTTP/1.0 request through a verified TLS client session:

```bash
printf 'GET / HTTP/1.0\r\nHost: example.com\r\n\r\n' | tlsio client example.com 443
```

Use a custom trust chain when the peer is not signed by the system trust store:

```bash
tlsio client 127.0.0.1 8443 --name localhost --ca ca.pem
```

Accept one TLS server connection:

```bash
tlsio server 8443 --cert server.pem --key server.key
```

Received plaintext is written to stdout. Data written to stdin is encrypted and
sent to the connected peer.

## Usage

```text
Usage: tlsio client <host> <port> [options]
       tlsio server <port> --cert <file> --key <file> [options]

Commands:
  client  Connect one authenticated TLS client session
  server  Accept one authenticated TLS server session

Client options:
  --name <host>  TLS server name for SNI and verification (default: host)
  --ca <file>    Trusted CA chain in PEM or DER (default: system trust)

Server options:
  --bind <host>  Local bind host (default: all interfaces)
  --cert <file>  Server certificate chain in PEM or DER
  --key <file>   Server private key in PEM or DER

Common options:
  -h, --help     Show this help
  -v, --version  Show version
```

Client mode uses `<host>` as both the TCP destination and the TLS server name by
default. `--name` changes only SNI and certificate hostname verification, which
allows the TCP destination and authenticated TLS identity to differ.

`--ca` replaces system trust with the supplied PEM or DER CA chain for that
client session.

Server mode accepts one TCP connection and creates one TLS server session for
it. `--bind` limits the local interface; when omitted, the listener uses the
available wildcard address. The certificate chain and private key may be PEM or
DER.

The relay ends successfully after an authenticated TLS `close_notify`. A TCP
close without TLS shutdown is treated as a transport failure instead of a clean
TLS session end.

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
