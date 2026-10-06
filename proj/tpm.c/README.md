# tpm

`tpm` compares input text with a representative text profile and prints a
similarity score as a single number in the range `0..1`:

```text
0.927314814815
```

A score near `1` means the input text is more similar to the map profile. A
score near `0` means it is less similar.

## Usage

```text
Usage: tpm <map> [text] [options]

Options:
  -i, --input <file>   Read input text from file
  -n, --ngram <N>      N-gram size (default: 3)
  -h, --help           Show this help
  -v, --version        Show version
```

`<map>` is the representative text used to build the profile. It is a literal
positional value, not a filename. Quote it when it contains spaces:

```bash
tpm "the quick brown fox jumps over the lazy dog" "the quick brown fox"
```

If input text is available from more than one source, `tpm` uses this order:

```text
text argument > stdin > -i/--input
```

Read the input from standard input:

```bash
printf 'the quick brown fox' | tpm "the quick brown fox jumps over the lazy dog"
```

Read the input from a file:

```bash
tpm "the quick brown fox jumps over the lazy dog" -i document.txt
```

Use a different n-gram size:

```bash
tpm "the quick brown fox jumps over the lazy dog" "the quick brown fox" --ngram 4
```

The n-gram size defaults to `3` and must be a positive integer.

An empty input is valid and produces a score of `0`. An empty profile is also
valid and produces a score of `0`.

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
