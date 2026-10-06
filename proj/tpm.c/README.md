# tpm

`tpm` is a command-line frontend for `libtpm.c` text profile matching.

It builds a profile from representative map text, scores one input text against
that profile, and prints the resulting similarity score as a single number in
the range `0..1`:

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
  -n, --ngram <N>      N-gram size 1..8 (default: 3)
  -h, --help           Show this help
  -v, --version        Show version
```

`<map>` is the representative text used to build the `libtpm` profile. It is a
literal positional value, not a filename. Quote it when it contains spaces:

```bash
tpm "the quick brown fox jumps over the lazy dog" "the quick brown fox"
```

The input text follows the standard KCCLI content precedence:

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

The n-gram size defaults to `3` and must be between `1` and `8`, matching the
public `libtpm.c` contract.

An empty input is valid and produces a score of `0`. A successfully created
empty profile is also valid according to `libtpm.c`.

## Build

The CLI links against the precompiled static `libtpm.c` artifacts in
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
