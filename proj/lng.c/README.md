# lng

`lng` is a command-line frontend for `liblng.c` language detection.

It accepts one text argument, standard input, or a file, then prints one ranked
result per line as headerless CSV:

```text
<code>,<score>
```

Scores are the heuristic ranking values returned by `liblng`; they are not
calibrated probabilities.

## Usage

```text
Usage: lng [text] [options]

Options:
  -i, --input <file>   Read text from file
  -t, --threshold <N>  Minimum score in range 0..1 (default: 0.001)
  -l, --limit <N>      Maximum results (default: 1, max: 32)
  -h, --help           Show this help
  -v, --version        Show version
```

Input precedence is:

```text
argument > stdin > -i/--input
```

A positional argument wins over redirected or piped stdin. Without an argument,
stdin wins over `-i` when stdin contains input. An empty redirected stdin may
fall through to the file.

Quote text containing spaces:

```bash
lng "Hello world"
```

Read from standard input:

```bash
printf 'Bonjour le monde' | lng
```

Read from a file:

```bash
lng -i document.txt
```

Return more ranked matches:

```bash
lng "Hello world" --limit 3
```

Filter weak matches:

```bash
lng "Hello world" --threshold 0.1
```

An empty input is valid and produces no output. Zero matches are also a
successful result.

`--limit` must be greater than zero. `liblng` clamps limits above 32 to 32.
`--threshold` must be finite and between 0 and 1 inclusive.

## Build

The CLI links against the precompiled static `liblng.c` artifacts in
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
