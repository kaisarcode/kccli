# mdp

`mdp` is a command-line frontend for `libmdp.c`.

It parses Markdown, exposes raw frontmatter and body views, and renders the body
as an HTML fragment.

## Usage

```text
Usage: mdp [text] [options]

Options:
  --body         Print Markdown body without frontmatter
  --meta         Print raw frontmatter
  -h, --help     Show this help
  -v, --version  Show version
```

HTML output is the default.

A positional argument is treated as literal Markdown text. When a positional
argument is present it takes priority over standard input. When it is absent,
input is read from standard input.

Render Markdown passed as an argument:

```bash
mdp "# Hello"
```

Render Markdown from standard input:

```bash
printf '# Hello' | mdp
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
