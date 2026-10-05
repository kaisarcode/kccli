# min

`min` is a command-line frontend for `libmin.c`.

It minifies CSS, JavaScript, HTML, or generic text and writes the result directly
to standard output.

## Usage

```text
Usage: min -t <type> [file]

Options:
  -t, --type <type>  css, js, html, or txt
  -h, --help         Show this help
  -v, --version      Show version
```

When `file` is omitted, input is read from standard input. `-` also explicitly
selects standard input.

Minify a CSS file:

```bash
min -t css style.css > style.min.css
```

Minify JavaScript from standard input:

```bash
cat app.js | min -t js > app.min.js
```

Minify HTML:

```bash
min -t html page.html > page.min.html
```

Collapse generic text whitespace:

```bash
printf '  hello   world  ' | min -t txt
```

The four modes map directly to `kc_min_css()`, `kc_min_js()`,
`kc_min_html()`, and `kc_min_txt()`. Minification behavior is defined by
`libmin.c`; the CLI does not add another parser or transformation layer.

## Build

The CLI links against the precompiled static `libmin.c` artifacts in
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
