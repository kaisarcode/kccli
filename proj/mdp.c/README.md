# mdp

`mdp` parses Markdown and prints HTML, ANSI terminal output, the Markdown body,
or raw frontmatter.

## Quick start

Render Markdown as HTML:

```bash
mdp "# Hello"
```

Render for a terminal:

```bash
mdp --ansi "# Hello"
```

## Usage

```text
Usage: mdp [text] [options]

Options:
  -i, --input <file>  Read Markdown from file when stdin is absent
  --ansi              Render Markdown for ANSI terminals
  --body              Print Markdown body without frontmatter
  --meta              Print raw frontmatter
  -h, --help          Show this help
  -v, --version       Show version
```

## Input and output

Markdown input is selected in this order:

```text
argument > stdin > -i/--input
```

A positional argument is literal Markdown. Redirected or piped stdin takes
priority over the input file when no positional argument is present.

HTML is the default output. `--ansi`, `--body`, and `--meta` are mutually
exclusive.

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
