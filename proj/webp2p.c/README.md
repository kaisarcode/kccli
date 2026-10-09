# webp2p.c - REDP2P WebRTC CLI

`webp2p` is the native WebRTC/DataChannel peer implementation for REDP2P.
It is currently hosted in the `kccli` monorepo, but is intentionally kept
independent from `kclib` so it can move under the REDP2P project later.

The program exists primarily to provide the WebRTC peer path on Linux desktop
systems where the embedded WebKit runtime does not provide the required WebRTC
support. Browser peers use the JavaScript REDP2P implementation instead.

`webp2p` talks to the same REDP2P indexes as the browser implementation. It
does not link `libredp2p.a` and does not add libdatachannel to `kclib`.

## Dependencies

`webp2p` builds libdatachannel from an external source checkout and also needs
several development packages from the host system.

### System packages

On Debian and Ubuntu, install the required build tools and development headers:

```sh
sudo apt update
sudo apt install \
  build-essential \
  cmake \
  pkg-config \
  libcurl4-openssl-dev \
  libssl-dev
```

`libcurl4-openssl-dev` provides the libcurl development files used for REDP2P
index HTTP requests. `libssl-dev` provides the OpenSSL development files used
by the REDP2P authentication and proof code.

### libdatachannel

libdatachannel is intentionally not vendored into this repository. Clone it
separately, including its submodules:

```sh
git clone --recursive https://github.com/paullouisageneau/libdatachannel.git
```

If the repository was already cloned without submodules, initialize them with:

```sh
cd libdatachannel
git submodule update --init --recursive
```

Point `webp2p` at that source tree with `LIBDATACHANNEL_DIR`:

```sh
export LIBDATACHANNEL_DIR="$HOME/work/vendor/libdatachannel"
```

The directory must contain libdatachannel's `CMakeLists.txt` and initialized
submodules. The source checkout remains external to `kccli`.

During the build, `webp2p` configures and builds a minimal static libdatachannel
stack inside `proj/webp2p.c/.build`. The external source tree is not modified or
copied into this repository.

The libdatachannel configuration disables media, WebSocket support, examples,
tests, GnuTLS, Mbed TLS, and libnice. It uses the libjuice ICE backend and the
static libdatachannel/usrsctp stack.

## Build

From the `kccli` repository root:

```sh
./scripts/build.sh webp2p
```

Or directly from the project directory:

```sh
cd proj/webp2p.c
make
```

Build all currently configured targets:

```sh
make all
```

The native binary is written to:

```text
bin/<arch>/linux/webp2p
```

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
--force-turn <0|1>     Disable or require TURN relay
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
TCP sessions use ordered DataChannels.

## Environment

Runtime fallback variables:

```text
REDP2P_PASS
REDP2P_STUN
REDP2P_TURN
REDP2P_TURN_USER
REDP2P_TURN_PASS
REDP2P_FORCE_TURN
```

Command-line values take precedence. `REDP2P_FORCE_TURN` accepts `0` or `1`.
Forcing TURN requires a TURN URL.

Build-time variable:

```text
LIBDATACHANNEL_DIR
```

## Tests

Run the native CLI contract tests:

```sh
make test
```

The ordinary tests exercise the CLI contract and do not require a public index.
The build itself requires the external libdatachannel checkout.

The standard kccli compatibility target is also accepted:

```sh
make test wine
```

There is no Windows `webp2p` target yet, so the Wine form currently reports a
skip instead of building a Windows binary.

## Scope

The initial implementation intentionally targets the Linux desktop gap that
motivated `webp2p`. It provides TCP service forwarding first. The REDP2P RTC
signaling and binary DataChannel layers are separated from the CLI forwarding
code so additional local transport behavior can be added without coupling it
to libdatachannel internals.

---

## License

This project is distributed under the GNU General Public License version 3.
