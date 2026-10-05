# emb

`emb` is a command-line frontend for `libemb.c` fixed-model text embeddings.

It accepts one text argument, standard input, or a file and prints one
headerless CSV vector per successful embedding.

The embedding dimension is fixed by the model embedded in `libemb.c`; callers
do not choose it.

## Usage

```text
Usage: emb [text] [options]

Options:
  -i, --input <file>  Read text from file
  -d, --dimension     Print embedding dimension
  -h, --help          Show this help
  -v, --version       Show version
```

Input precedence is:

```text
argument > stdin > -i/--input
```

A positional argument wins over redirected or piped stdin. Without an argument,
stdin wins over `-i` when stdin contains input. An empty redirected stdin may
fall through to the file.

Embed one text:

```bash
emb "The quick brown fox"
```

Read text from standard input:

```bash
printf 'The quick brown fox' | emb
```

Read text from a file:

```bash
emb -i document.txt
```

Output is one headerless CSV row:

```text
-0.0123,0.0441,-0.0087,...
```

Print the fixed model dimension:

```bash
emb --dimension
```

The current embedded model is BAAI/bge-small-en-v1.5. The dimension is a
library property and should be queried rather than hardcoded.

## Build

The CLI links against the precompiled static `libemb.c` artifacts in
`kclib/dist`. Because the static library contains GGML C++ objects, the CLI
source remains C but the final link uses the target C++ linker.

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
