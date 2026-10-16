# min

`min` minifies CSS, JavaScript, HTML, or generic text and writes the result to
standard output.

## Quick start

```bash
min --type css "body { color: red; }"
```

Generic text is the default type:

```bash
printf '  hello   world  ' | min
```

## Usage

```text
Usage: min [text] [options]

Options:
  -i, --input <file>  Read input from file
  -t, --type <type>   css, js, html, or txt (default: txt)
  -h, --help          Show this help
  -v, --version       Show version
```

Input is selected in this order:

```text
argument > stdin > -i/--input
```

The positional argument is literal source text. Supported type names are `css`,
`js`, `html`, and `txt`.

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
