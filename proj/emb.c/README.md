# emb

`emb` generates text embeddings with the embedded BGE small v1.5 model and
prints one headerless CSV vector.

## Quick start

```bash
emb "The quick brown fox"
```

```text
-0.0123,0.0441,-0.0087,...
```

Query the model dimension instead of hardcoding it:

```bash
emb --dimension
```

## Usage

```text
Usage: emb [text] [options]

Options:
  -i, --input <file>  Read text from file
  -d, --dimension     Print embedding dimension
  -h, --help          Show this help
  -v, --version       Show version
```

## Input and output

Text is selected in this order:

```text
argument > stdin > -i/--input
```

A positional argument wins over redirected or piped stdin. Without an argument,
stdin wins over `--input` when it contains data. An empty redirected stdin may
fall through to the file.

Each successful embedding is written as one CSV row with no header.

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
