# tpm

`tpm` compares input text with a representative text profile and prints one
similarity score from `0` to `1`.

## Quick start

```bash
tpm "the quick brown fox jumps over the lazy dog" "the quick brown fox"
```

```text
0.927314814815
```

A score near `1` indicates greater similarity to the representative text.

## Usage

```text
Usage: tpm <map> [text] [options]

Options:
  -i, --input <file>   Read input text from file
  -n, --ngram <N>      N-gram size (default: 3)
  -h, --help           Show this help
  -v, --version        Show version
```

`<map>` is literal representative text, not a filename. Quote it when it
contains spaces.

Input text is selected in this order:

```text
text argument > stdin > -i/--input
```

The n-gram size must be a positive integer. Empty input and an empty profile are
valid and produce a score of `0`.

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
