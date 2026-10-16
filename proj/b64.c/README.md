# b64

`b64` encodes binary or text input as RFC 4648 Base64 and decodes strict
canonical Base64 input. Results are written directly to standard output.

## Quick start

Encode text:

```bash
b64 --encode "hello world"
```

```text
aGVsbG8gd29ybGQ=
```

Decode it again:

```bash
b64 --decode aGVsbG8gd29ybGQ=
```

## Usage

```text
Usage: b64 -e [text] [options]
       b64 -d [base64] [options]

Options:
  -e, --encode        Encode as Base64
  -d, --decode        Decode Base64
  -i, --input <file>  Read input from file
  -h, --help          Show this help
  -v, --version       Show version
```

Exactly one of `--encode` or `--decode` is required.

## Input and output

Input is selected in this order:

```text
argument > stdin > -i/--input
```

The positional argument is literal input. Redirected or piped stdin takes
priority over `--input` when no positional argument is present. An empty
redirected stdin may fall through to the input file.

Encoding from stdin or a file is binary-safe. Decoded bytes are written raw to
standard output.

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
