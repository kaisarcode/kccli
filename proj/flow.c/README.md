# flow

`flow` opens a `.flow` document, applies temporary ordered overrides, runs it,
and writes successful output to stdout.

## Usage

```text
Usage: flow <file.flow> [options]

Options:
  -e, --entry <node>       Start from one entry node
  -i, --input <path>       Input file; use '-' for stdin
  -s, --set <key> <value>  Set an override; repeatable
  -u, --unset <key>        Remove an override; repeatable
  -h, --help               Show this help
  -v, --version            Show version
```

`--set` and `--unset` may be repeated. Their order is preserved.

If `--entry` is omitted, the declared flow entries are used.

If `--input` is omitted, the run receives no input buffer. Use `-i -` to read
input from stdin:

```bash
printf 'hello' | flow example.flow -i -
```

A binary input file can be supplied directly:

```bash
flow example.flow --input payload.bin
```

Successful output is written unchanged to stdout. Diagnostics are written to
stderr.

## Overrides

Overrides apply only to the current invocation and do not modify the source
`.flow` file.

```bash
flow example.flow \
    --set flow.name Alice \
    --unset flow.debug \
    --set flow.mode production
```

The operations above are applied in exactly that order.

## Entry

Run one specific entry node:

```bash
flow example.flow --entry build
```

## Cancellation

`Ctrl+C` and termination signals request cancellation. The command currently
running is not killed; the current step completes and the run stops before
starting another step.

A cooperatively stopped flow exits with status `130`.

## Build

```bash
make
```

Build all configured desktop targets:

```bash
make all
```

## Tests

```bash
make test
```

Run the Windows tests through Wine:

```bash
make test wine
```

## Platforms

Configured desktop/server targets include Linux, Windows, and macOS.

No external runtime services are required.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
