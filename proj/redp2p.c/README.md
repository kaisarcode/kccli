# redp2p.c - REDP2P CLI

`redp2p` runs REDP2P index, publisher, and consumer commands from the command
line. Publisher and consumer commands bridge REDP2P traffic to local ports.

## Build

```sh
make
make all
```

## Tests

```sh
make test
make test wine
```

## CLI

Start an index:

```bash
redp2p idx 9876
redp2p idx 9876 --seats 128 --pow 16
```

List active publisher IDs from a local index:

```bash
redp2p idx 9876 --list
```

Publish a local TCP or UDP port:

```bash
redp2p pub web@idx.example.com:9876 --tcp 8080
redp2p pub game@idx.example.com:9876 --udp 7777
```

Expose one announced publisher through a local port:

```bash
redp2p con web@idx.example.com:9876 9000
```

The consumer derives TCP or UDP from the publisher record. It therefore does
not accept `--tcp` or `--udp`.

The CLI accepts `REDP2P_SEATS`, `REDP2P_POW`, `REDP2P_PASS`,
`REDP2P_VIP`, `REDP2P_MAX_CONSUMERS_PER_PUBLISHER`, and `REDP2P_STUN`
as operator configuration and translates them into the public API.

`idx`, `pub`, and `con` processes run until SIGINT or SIGTERM and then
close their handle.

Common options are `-h` / `--help` and `-v` / `--version`.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
