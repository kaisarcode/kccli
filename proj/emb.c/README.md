# emb

`emb` is a command-line frontend for `libemb.c` fixed-model text embeddings.

It accepts one text argument or reads text from standard input. Successful
embedding output is CSV without a header: one vector per line, with float
values separated by commas.

The embedding dimension is fixed by the model embedded in `libemb.c`; callers
do not choose it.

## Usage

```text
Usage: emb [text] [options]

Options:
  -d, --dimension  Print embedding dimension
  -h, --help       Show this help
  -v, --version    Show version
```

Embed one text:

```bash
emb "The quick brown fox"
```

Read text from standard input:

```bash
printf 'The quick brown fox' | emb
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
