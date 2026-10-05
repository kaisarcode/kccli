# tpl

`tpl` is a command-line frontend for `libtpl.c` template rendering.

It loads one template source, applies an isolated variable set, and writes the
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

Input precedence is:

```text
stdin > positional argument > -i/--input
```

Standard input takes precedence only when it is redirected or piped and contains
bytes. An empty redirected stdin falls through to the positional argument and
then to `-i`.

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

`--var` is repeatable. Variables are passed to one `kc_tpl_render()` call as an
isolated `kc_tpl_var_t` array.

The output is written unchanged to standard output. Rendering errors are written
to standard error and return a nonzero process status.

The template syntax, escaping, includes, blocks, conditionals, and foreach
behavior are implemented by `libtpl.c`; the CLI does not add another template
language or preprocessing layer.

## Build

The CLI links against the precompiled static `libtpl.c` artifacts in
`kclib/dist`.

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
