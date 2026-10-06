# tpl

`tpl` renders templates with variables and optional includes.

It loads one template source, applies the provided variables, and writes the
rendered result directly to standard output.

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

A positional argument is literal template text. A template file is selected
only with `-i` or `--input`.

If template input is available from more than one source, `tpl` uses this order:

```text
argument > stdin > -i/--input
```

A positional argument wins over redirected or piped stdin. Without an argument,
stdin wins over `-i` when stdin contains input. An empty redirected stdin may
fall through to the file.

Render literal template text:

```bash
tpl '<h1>{{ title }}</h1>' --var title Home
```

Render from standard input:

```bash
printf '<h1>{{ title }}</h1>' | tpl --var title Home
```

Render a template file:

```bash
tpl -i page.tpl --var title Home --var user_name Kaisar
```

Set a custom include root:

```bash
tpl -i page.tpl --root ./views
```

`--var` is repeatable. The output is written unchanged to standard output.
Rendering errors are written to standard error and return a nonzero process
status.

Supported template features include escaping, includes, blocks, conditionals,
and foreach loops.

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

## License

GNU General Public License version 3 or later.
