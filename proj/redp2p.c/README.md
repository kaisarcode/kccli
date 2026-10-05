# redp2p.c - REDP2P CLI

`redp2p` runs REDP2P index, publisher, and consumer commands from the command
line.

A publisher exposes a local TCP or UDP service through REDP2P. A consumer makes
that published service available on a local port.

## Build

Build the native host target:

```sh
make
```

Build all configured Linux, Windows, and macOS targets:

```sh
make all
```

## Tests

Run the native CLI tests:

```sh
make test
```

The native test run includes the CLI argument contract.

Run the x86_64 Windows CLI contract through Wine:

```sh
make test wine
```

## CLI

Start an index:

```bash
redp2p idx 9876
redp2p idx 9876 --seats 128 --pow 16
```

Publish a local TCP service. TCP is the default transport:

```bash
redp2p pub web@idx.example.com:9876 -p 8080
redp2p pub web@idx.example.com:9876 --port 8080 --tcp
```

Publish a local UDP service:

```bash
redp2p pub game@idx.example.com:9876 -p 7777 --udp
```

Peer options may be combined with either transport:

```bash
redp2p pub web@idx.example.com:9876 -p 8080 \
  --turn turn:turn.example.com:3478 \
  --turn-user user --turn-pass secret
```

Expose one announced publisher through a local TCP port. TCP is also the
default consumer transport:

```bash
redp2p con web@idx.example.com:9876 -p 9000
redp2p con web@idx.example.com:9876 --port 9000 --tcp
```

Expose it through a local UDP port:

```bash
redp2p con game@idx.example.com:9876 -p 9000 --udp
```

The CLI accepts `REDP2P_SEATS`, `REDP2P_POW`, `REDP2P_PASS`, `REDP2P_VIP`,
`REDP2P_MAX_CONSUMERS_PER_PUBLISHER`, `REDP2P_STUN`, `REDP2P_TURN`,
`REDP2P_TURN_USER`, `REDP2P_TURN_PASS`, and `REDP2P_FORCE_TURN` as operator
configuration.

For `pub` and `con`, `-p` / `--port` is required. `--tcp` and `--udp` are
optional transport selectors; when neither is present, TCP is used. The peer
options are `--stun`, `--turn`, `--turn-user`, and `--turn-pass`. Direct peer
connectivity remains preferred; TURN is used by libredp2p as a relay fallback.
Set `REDP2P_FORCE_TURN=1` to require the relay path. The CLI validates the
switch, requires TURN configuration when it is enabled, and reports that the
forced relay mode is active.

`idx`, `pub`, and `con` processes run until SIGINT or SIGTERM and then close
their handle.

Common options are `-h` / `--help` and `-v` / `--version`.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
