# mdp

`mdp` is a command-line frontend for `libmdp.c`.

It parses Markdown, exposes raw frontmatter and body views, and renders the body
as an HTML fragment.

## Usage

```text
Usage: mdp [text] [options]

Options:
  -i, --input <file>  Read Markdown from file when stdin is absent
  --body              Print Markdown body without frontmatter
  --meta              Print raw frontmatter
  -h, --help          Show this help
  -v, --version       Show version
```

HTML output is the default.

Input precedence is:

```text
positional argument > stdin > -i/--input
```

A positional argument is treated as literal Markdown text. When it is present,
it is used even if stdin is redirected or `-i/--input` is also provided.

When no positional argument is present, redirected stdin takes priority over
`-i/--input`. The input file is used only when no higher-priority input is
available.

Render Markdown passed as an argument:

```bash
mdp "# Hello"
```

Render Markdown from standard input:

```bash
printf '# Hello' | mdp
```

Render a Markdown file:

```bash
mdp -i README.md
```

If both stdin and a file are provided, stdin wins:

```bash
printf '# From stdin' | mdp -i README.md
```

Print the Markdown body after recognized frontmatter:

```bash
printf '%s\n' '---' 'title: Home' '---' '# Hello' | mdp --body
```

Print raw frontmatter:

```bash
printf '%s\n' '---' 'title: Home' '---' '# Hello' | mdp --meta
```

`--body` and `--meta` are mutually exclusive. Without either option, the CLI
calls `kc_mdp_html()` and writes the resulting HTML fragment directly to
standard output.

## Build

The CLI links against precompiled static `libmdp.c` artifacts in `kclib/dist`.

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
