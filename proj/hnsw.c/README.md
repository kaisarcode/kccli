# hnsw

`hnsw` is a command-line frontend for `libhnsw.c`.

It builds one in-memory HNSW index from a vector dataset, runs one query, prints
the matches, and exits. It does not add persistence, storage, or a database
layer that the library itself does not provide.

## Usage

```text
Usage: hnsw <vectors> -q <query> -d <dimension> [options]

Options:
  -i, --input <file>       Read vectors from file
  -q, --query <vector>     Query vector as comma-separated floats
  -d, --dimension <N>      Vector dimension
  -m, --metric <metric>    cosine, inner, or l2 (default: cosine)
  -l, --limit <N>          Maximum results (default: 5)
  -t, --threshold <N>      Metric-specific result threshold
  --max-connections <N>    Maximum graph connections
  --build-effort <N>       Graph build effort
  --search-effort <N>      Search effort
  -h, --help               Show this help
  -v, --version            Show version
```

The vector dataset has exactly two logical columns per row, separated by one
tab character:

```text
<id><TAB><vector>
```

The vector column is a comma-separated list of floats. The identifier is a
separate field and is never interpreted as part of the vector.

For example:

```text
alice	1,0,0
bob	0,1,0
carol	0,0,1
```

The positional dataset argument is literal dataset content, not a filename.
File input is explicit through `-i` or `--input`.

Input precedence follows the KCCLI content convention:

```text
argument > stdin > -i/--input
```

Search literal vectors:

```bash
hnsw $'a\t1,0\nb\t0,1' -q 1,0 -d 2
```

Search vectors from standard input:

```bash
printf 'a\t1,0\nb\t0,1\n' | hnsw -q 1,0 -d 2
```

Search vectors from a file:

```bash
hnsw -i vectors.tsv -q 1,0 -d 2
```

Output is headerless CSV:

```text
<id>,<score>
```

The CLI creates the index, adds every input row, calls `kc_hnsw_build()`, calls
`kc_hnsw_search()`, prints the results, and releases the index in one process.
It deliberately does not provide save/load commands because `libhnsw.c` is an
in-memory library.

Available metrics are:

```text
cosine
inner
l2
```

`inner_product` is also accepted as an alias for `inner`.

For cosine and inner product, `--threshold` is a minimum accepted score. For
L2 it is a maximum accepted squared distance. If omitted, the CLI applies an
unbounded threshold appropriate to the selected metric.

The tuning options map directly to `kc_hnsw_options_t` and otherwise retain the
library defaults.

## Build

The CLI links against precompiled static `libhnsw.c` artifacts in `kclib/dist`.

```bash
make
```

Build all configured native targets:

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
