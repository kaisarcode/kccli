# tlsio

`tlsio` is a binary TLS relay for one TCP connection. It can run as a TLS
client or as a one-connection TLS server, sending standard input as application
data and writing authenticated peer plaintext to standard output.

The CLI does not implement HTTP or another application protocol. It provides a
TLS-protected byte stream that can be composed with shell commands, files, or
other processes.

## Quick start

Send an HTTP/1.0 request through a verified TLS client session:

```bash
printf 'GET / HTTP/1.0\r\nHost: example.com\r\n\r\n' | tlsio client example.com 443
```

The TCP destination is `example.com:443`. By default, the same host is used for
SNI and certificate hostname verification.

## Usage

```text
tlsio client <host> <port> [options]
tlsio server <port> --cert <file> --key <file> [options]
```

Client options:

- `--name <host>` sets the TLS server name used for SNI and hostname
    verification. The default is the TCP destination host.
- `--ca <file>` uses the supplied PEM or DER CA chain instead of the system
    trust store.

Server options:

- `--bind <host>` limits the local bind address. The default is the platform
    wildcard address.
- `--cert <file>` supplies the server certificate chain in PEM or DER form.
- `--key <file>` supplies the server private key in PEM or DER form.

Common options:

- `-h, --help` shows command help.
- `-v, --version` prints the linked library build version.

## Client identity

Client mode normally uses `<host>` for both the TCP destination and the TLS
server identity. Use `--name` when those values differ, such as when connecting
to a local address while verifying a certificate for another hostname:

```bash
tlsio client 127.0.0.1 8443 --name localhost --ca ca.pem
```

Without `--ca`, the client uses the platform trust store. With `--ca`, only the
supplied CA chain is used for that session.

## Server mode

Server mode accepts one TCP connection, runs one TLS session, and then exits.
The server authenticates itself with the certificate and private key supplied by
`--cert` and `--key`.

```bash
tlsio server 8443 --cert server.pem --key server.key
```

Use `--bind` to restrict the listener to a specific local address:

```bash
tlsio server 8443 --bind 127.0.0.1 --cert server.pem --key server.key
```

Server mode does not request or verify a client certificate.

## Standard streams

Standard input is treated as binary plaintext to send through TLS. Authenticated
plaintext received from the peer is written unchanged to standard output.
Diagnostics are written to standard error.

Reaching EOF on standard input stops local application writes but keeps the TLS
session open for peer data. The relay exits successfully when the peer sends an
authenticated TLS `close_notify`. A TCP close without TLS shutdown is reported
as a transport failure.

## Build

The Makefile requires `KCLIB_DIR` to point to the checkout containing the
matching prebuilt `libtlsio` artifacts and public header.

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
