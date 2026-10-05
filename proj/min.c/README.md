# min

`min` is a command-line frontend for `libmin.c`.

It minifies CSS, JavaScript, HTML, or generic text and writes the result directly
to standard output.

## Usage

```text
Usage: min [text] [options]

Options:
  -i, --input <file>  Read input from file
  -t, --type <type>   css, js, html, or txt (default: txt)
  -h, --help          Show this help
  -v, --version       Show version
```

The positional argument is literal source text. File input is selected with
`-i` or `--input`.

Input precedence is:

```text
argument > stdin > -i/--input
```

A positional argument wins over redirected or piped stdin. Without an argument,
stdin wins over `-i` when stdin contains input. An empty redirected stdin may
fall through to the file.

Generic text is the default mode:

```bash
min "fdfsdfsd sdfsd     ff sd              fsdfd"
```

Output:

```text
fdfsdfsd sdfsd ff sd fsdfd
```

Read from standard input:

```bash
printf '  hello   world  ' | min
```

Read from a file:

```bash
min -i input.txt
```

Select another minifier with `-t` or `--type`:

```bash
min -t css "body { color: red; }"
min -t js "const x = 1; // comment"
min -t html "<!-- comment --><p>hello</p>"
min -t css -i style.css
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
