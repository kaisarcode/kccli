# ngram

`ngram` traverses descending sliding windows over text tokens and prints each
visited chunk to standard output.

Windows are visited from largest to smallest. Windows of the same size are
visited from left to right.

## Usage

```text
Usage: ngram [text] [options]

Options:
  -i, --input <file>       Read text from file
  -f, --filter <cmd>       Run a filter command for each chunk
      --max <N>            Maximum tokens per chunk (default: 10)
      --min <N>            Minimum tokens per chunk (default: 1)
  -s, --separators <chars> Token separator characters
  -h, --help               Show this help
  -v, --version            Show version
```

The default separators are space, tab, carriage return, and newline.
`--separators` replaces that set. An empty separator string treats the input as
one token.

`--max 0` uses all available tokens as the largest window. A nonzero `--max`
must not be smaller than `--min`. `--min` must be greater than zero.

If text is available from more than one source, `ngram` uses this order:

```text
text argument > stdin > -i/--input
```

Traverse literal text:

```bash
ngram "the quick brown fox" --max 2 --min 1
```

Read from standard input:

```bash
printf 'the quick brown fox' | ngram --max 3
```

Read from a file:

```bash
ngram -i document.txt --max 3
```

Use commas as token separators:

```bash
ngram "one,two,three" --separators ","
```

## Filtering

`--filter` runs one command synchronously for every visited chunk. The chunk is
written to the command's standard input followed by a newline.

If the filter writes anything to standard output, the current span is closed.
Smaller windows fully contained in that span are skipped. If the filter writes
nothing, traversal continues normally through that span.

The filter's standard output is consumed only as the decision signal and is not
copied to `ngram` output. The filter's exit status does not select or reject a
chunk.

For example, close a span whenever `grep` finds `brown`:

```bash
ngram "the quick brown fox jumps" --filter "grep brown"
```

Each chunk is printed by `ngram` before its filter decision is applied. The
filter therefore controls which smaller chunks are visited later; it does not
remove the current chunk from output.

Without `--filter`, every candidate chunk in the configured traversal range is
printed.

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

Windows tests can be run through Wine:

```bash
make test wine
```

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
