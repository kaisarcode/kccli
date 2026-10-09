# webp2p.c - REDP2P WebRTC CLI

`webp2p` is the native WebRTC/DataChannel peer implementation for REDP2P.
It is currently hosted in the `kccli` monorepo, but is intentionally kept
independent from `kclib` so it can move under the REDP2P project later.

The program exists primarily to provide the WebRTC peer path on Linux desktop
systems where the embedded WebKit runtime does not provide the required WebRTC
support. Browser peers use the JavaScript REDP2P implementation instead.

`webp2p` talks to the same REDP2P indexes as the browser implementation. It
does not link `libredp2p.a` and does not add libdatachannel to `kclib`.

## Dependency

The libdatachannel source checkout is external to this repository. Set
`LIBDATACHANNEL_DIR` to a checkout that includes its submodules:

```sh
export LIBDATACHANNEL_DIR="$HOME/work/vendor/libdatachannel"
```

`make` configures and builds the minimal static DataChannel stack inside this
project's `.build` directory. The external libdatachannel source tree is not
copied into `kccli`.

The current Linux build also requires CMake, a C/C++ toolchain, pkg-config,
OpenSSL development files, and libcurl development files.

## Build

```sh
make
```

The native binary is written to:

```text
bin/<arch>/linux/webp2p
```

The libdatachannel configuration disables media, WebSocket support, examples,
tests, GnuTLS, Mbed TLS, and libnice. It uses the libjuice ICE backend and the
static libdatachannel/usrsctp stack.

## CLI

Publisher:

```text
webp2p pub <id>@<index-url> -p <port> [options]
```

Consumer:

```text
webp2p con <id>@<index-url> -p <port> [options]
```

The index is an HTTP or HTTPS REDP2P index URL. The local transport is currently
TCP; `-t tcp` / `--transport tcp` is accepted so the command shape can grow to
other local transports without changing the basic interface.

Publisher options:

```text
-p, --port <port>      Local TCP service port
-t, --transport tcp    Local transport
--pass <pass>          Index registration password
--stun <url>           Optional STUN URL
--turn <url>           Optional TURN URL
--turn-user <user>     Optional TURN username
--turn-pass <pass>     Optional TURN password
```

Consumer options are the same except `--pass`, which is publisher-only.

Examples:

```sh
webp2p pub demo@https://index.redp2p.com:9000/ -p 8080
webp2p con demo@https://index.redp2p.com:9000/ -p 9000
```

A publisher registers with transport `rtc`, polls for pending WebRTC offers,
answers them, and forwards each opened DataChannel to a separate connection to
the configured loopback TCP service. A consumer listens on the local port and
opens one REDP2P WebRTC/DataChannel connection per accepted local TCP session.

## Environment

Runtime fallback variables:

```text
REDP2P_PASS
REDP2P_STUN
REDP2P_TURN
REDP2P_TURN_USER
REDP2P_TURN_PASS
```

Command-line values take precedence.

Build-time variable:

```text
LIBDATACHANNEL_DIR
```

## Tests

```sh
make test
```

The ordinary tests exercise the CLI contract and do not require a public index.
The build itself requires the external libdatachannel checkout.

## Scope

The initial implementation intentionally targets the Linux desktop gap that
motivated `webp2p`. It provides TCP service forwarding first. The REDP2P RTC
signaling and binary DataChannel layers are separated from the CLI forwarding
code so additional local transport behavior can be added without coupling it
to libdatachannel internals.

---

## License

This project is distributed under the GNU General Public License version 3.
