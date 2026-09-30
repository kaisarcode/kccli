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

Run the native CLI contract:

```sh
make test
```

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

Publish a local TCP or UDP port:

```bash
redp2p pub web@idx.example.com:9876 --tcp 8080
redp2p pub game@idx.example.com:9876 --udp 7777
redp2p pub web@idx.example.com:9876 --tcp 8080 \
  --turn turn:turn.example.com:3478 \
  --turn-user user --turn-pass secret
```

Expose one announced publisher through a local port:

```bash
redp2p con web@idx.example.com:9876 9000
redp2p con web@idx.example.com:9876 9000 \
  --turn turn:turn.example.com:3478 \
  --turn-user user --turn-pass secret
```

The consumer does not accept `--tcp` or `--udp`; the publisher transport is
handled by REDP2P.

The CLI accepts `REDP2P_SEATS`, `REDP2P_POW`, `REDP2P_PASS`, `REDP2P_VIP`,
`REDP2P_MAX_CONSUMERS_PER_PUBLISHER`, `REDP2P_STUN`, `REDP2P_TURN`,
`REDP2P_TURN_USER`, and `REDP2P_TURN_PASS` as operator configuration.

For `pub` and `con`, the equivalent command-line peer options are
`--stun`, `--turn`, `--turn-user`, and `--turn-pass`. Direct peer
connectivity remains preferred; TURN is used by libredp2p as a relay fallback.

`idx`, `pub`, and `con` processes run until SIGINT or SIGTERM and then close
their handle.

Common options are `-h` / `--help` and `-v` / `--version`.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
