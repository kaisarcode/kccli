# tpl

`tpl` is a command-line frontend for `libtpl.c` template rendering.

It loads one template source, applies an isolated variable set, and writes the
rendered result directly to standard output.

## Usage

```text
Usage: tpl <template> [options]

Options:
  -r, --root <path>       Include root (default: .)
  -v, --var <key> <value> Set a template variable; repeatable
  -h, --help              Show this help
  -V, --version           Show version
```

Render a file:

```bash
tpl page.tpl --var title Home --var user_name Kaisar
```

Read the template source from standard input:

```bash
printf '<h1>{{ title }}</h1>' | tpl - --var title Home
```

Set a custom include root:

```bash
tpl page.tpl --root ./views
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
