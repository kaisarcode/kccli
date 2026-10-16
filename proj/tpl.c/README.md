# tpl

`tpl` renders one template with variables and optional includes, then writes the
rendered result directly to standard output.

## Quick start

```bash
tpl '<h1>{{ title }}</h1>' --var title Home
```

Render a file and set multiple variables:

```bash
tpl -i page.tpl --var title Home --var user_name Kaisar
```

## Usage

```text
Usage: tpl [template] [options]

Options:
  -i, --input <file>      Read template from file
  -r, --root <path>       Include root (default: .)
  -v, --var <key> <value> Set a template variable; repeatable
  -h, --help              Show this help
  -V, --version           Show version
```

Template input is selected in this order:

```text
argument > stdin > -i/--input
```

A positional argument is literal template text. `--var` is repeatable and
`--root` controls the directory used to resolve includes.

See [Template syntax](doc/template-syntax.md) for expressions, escaping,
includes, blocks, conditionals, and loops.

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
