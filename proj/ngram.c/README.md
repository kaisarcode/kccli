# ngram

`ngram` traverses descending sliding windows over text tokens and prints every
visited chunk to standard output.

## Quick start

```bash
ngram "the quick brown fox" --max 2 --min 1
```

Windows are visited from largest to smallest and left to right within each
size.

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

## Input and windows

Input is selected in this order:

```text
argument > stdin > -i/--input
```

The default separators are space, tab, carriage return, and newline.
`--separators` replaces that set. An empty separator string treats the entire
input as one token.

`--max 0` uses all available tokens as the largest window. A nonzero `--max`
must not be smaller than `--min`, and `--min` must be greater than zero.

See [Filtering traversal](doc/filtering.md) when using `--filter`; filter output
controls which smaller windows are visited and is not copied to normal output.

## Build

The Makefile requires `KCLIB_DIR` to point to the checkout that contains the
matching prebuilt library and public header.

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
