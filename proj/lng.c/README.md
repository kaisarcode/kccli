# lng

`lng` detects the language of input text and prints ranked matches as
headerless TSV.

## Quick start

```bash
lng "Hello world"
```

```text
en<TAB>0.99999
```

Scores are heuristic ranking values, not calibrated probabilities.

## Usage

```text
Usage: lng [text] [options]

Options:
  -i, --input <file>   Read text from file
  -t, --threshold <N>  Minimum score in range 0..1 (default: 0.001)
  -l, --limit <N>      Maximum results (default: 1, max: 32)
  -h, --help           Show this help
  -v, --version        Show version
```

## Input and results

Text is selected in this order:

```text
argument > stdin > -i/--input
```

An empty input is valid and produces no output. Zero matches are also
successful.

`--limit` must be greater than zero and values above 32 are limited to 32.
`--threshold` must be finite and between 0 and 1 inclusive.

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
