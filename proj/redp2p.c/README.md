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

### Index

```text
redp2p idx <port> [options]
```

Options:

```text
--seats <N>            Publisher capacity; default unlimited
--pow <N>              Registration PoW bits (0..32)
--max-consumers <N>    Consumers allowed per publisher
--pass <pass>          Registration password
--vip <id> <pass>      Reserve an id/password pair; repeatable
```

Examples:

```bash
redp2p idx 9876
redp2p idx 9876 --seats 128 --pow 16
redp2p idx 9876 --pass secret --vip admin vip-secret
```

### Publisher

```text
redp2p pub <id>@<index[:port]> -p <port> [options]
```

Options:

```text
-p, --port <port>      Local service port
--transport <type>     tcp or udp (default: tcp)
--pass <pass>          Index registration password
--stun <url>           Optional STUN URL
--turn <url>           Optional TURN URL
--turn-user <u>        Optional TURN username
--turn-pass <p>        Optional TURN password
--force-turn <0|1>     Disable or require TURN relay
```

Examples:

```bash
redp2p pub web@idx.example.com:9876 -p 8080
redp2p pub web@idx.example.com:9876 --port 8080 --transport tcp
redp2p pub game@idx.example.com:9876 -p 7777 --transport udp
redp2p pub web@idx.example.com:9876 -p 8080 --pass secret \
  --turn turn:turn.example.com:3478 \
  --turn-user user --turn-pass secret --force-turn 1
```

### Consumer

```text
redp2p con <id>@<index[:port]> -p <port> [options]
```

Options:

```text
-p, --port <port>      Local service port
--transport <type>     tcp or udp (default: tcp)
--stun <url>           Optional STUN URL
--turn <url>           Optional TURN URL
--turn-user <u>        Optional TURN username
--turn-pass <p>        Optional TURN password
--force-turn <0|1>     Disable or require TURN relay
```

Examples:

```bash
redp2p con web@idx.example.com:9876 -p 9000
redp2p con web@idx.example.com:9876 --port 9000 --transport tcp
redp2p con game@idx.example.com:9876 -p 9000 --transport udp
```

Common options are `-h` / `--help` and `-v` / `--version`.

## Environment compatibility

Command-line options are the public CLI surface. The following environment
variables remain supported as fallback/operator configuration for compatibility:

| Environment | CLI equivalent | Commands |
| --- | --- | --- |
| `REDP2P_SEATS` | `--seats <N>` | `idx` |
| `REDP2P_POW` | `--pow <N>` | `idx` |
| `REDP2P_PASS` | `--pass <pass>` | `idx`, `pub` |
| `REDP2P_VIP` | `--vip <id> <pass>` | `idx` |
| `REDP2P_MAX_CONSUMERS_PER_PUBLISHER` | `--max-consumers <N>` | `idx` |
| `REDP2P_STUN` | `--stun <url>` | `pub`, `con` |
| `REDP2P_TURN` | `--turn <url>` | `pub`, `con` |
| `REDP2P_TURN_USER` | `--turn-user <u>` | `pub`, `con` |
| `REDP2P_TURN_PASS` | `--turn-pass <p>` | `pub`, `con` |
| `REDP2P_FORCE_TURN` | `--force-turn <0|1>` | `pub`, `con` |

When both an environment value and its command-line equivalent are supplied,
the command-line value takes precedence.

For `pub` and `con`, `-p` / `--port` is required. `--transport` accepts `tcp`
or `udp`; when omitted, TCP is used. Direct peer connectivity remains
preferred; TURN is used by libredp2p as a relay fallback unless forced.

`idx`, `pub`, and `con` processes run until SIGINT or SIGTERM and then close
their handle.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
