# flow

`flow` is the kccli command-line projection of `libflow.c`.

It opens an existing `.flow` document, applies temporary ordered overrides,
starts one run, and writes the successful output to stdout.

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

`--set` and `--unset` may be repeated. Their order is preserved exactly and is
passed to the opened flow in that same order.

If `--entry` is omitted, the declared flow entries are used.

If `--input` is omitted, the run receives no input buffer. Use `-i -` to read
input from stdin, which makes the CLI convenient in pipelines:

```bash
printf 'hello' | flow example.flow -i -
```

A binary input file can be supplied directly:

```bash
flow example.flow --input payload.bin
```

Successful flow output is written unchanged to stdout. Diagnostics are written
to stderr.

## Overrides

Overrides are local to this invocation and do not modify the source `.flow`
file.

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

`Ctrl+C` and termination signals request `kc_flow_run_stop()`.

Cancellation is cooperative, matching `libflow.c`: the command currently
running is not killed. The current step completes and the run stops before
starting another step.

A cooperatively stopped flow exits with status `130`.

## Build

The CLI links directly against the precompiled static `libflow.c` artifact from
`kclib/dist`.

Build the native target:

```bash
make
```

Build all configured desktop targets:

```bash
make all
```

## Tests

Run native CLI contract tests:

```bash
make test
```

Run the Windows contract tests through Wine:

```bash
make test wine
```

From the kccli repository root the project is also discovered automatically:

```bash
./scripts/build.sh flow
./scripts/test.sh flow
```

## Platforms

The kccli project follows the desktop/server targets provided by the distributed
`libflow.c` static artifacts, including Linux, Windows, and macOS targets.

No external runtime services are required by the CLI itself.

## License

GNU General Public License version 3.
