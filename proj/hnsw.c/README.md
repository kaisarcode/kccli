# hnsw

`hnsw` builds an in-memory HNSW index from a vector dataset, runs one query,
prints the matches, and exits.

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

The vector dataset has exactly two columns per row, separated by one tab:

```text
<id><TAB><vector>
```

The vector is a comma-separated list of floats. For example:

```text
alice   1,0,0
bob 0,1,0
carol   0,0,1
```

The positional dataset argument is literal content, not a filename. File input
is selected with `-i` or `--input`.

If vector data is available from more than one source, `hnsw` uses this order:

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

The index exists only for the current invocation. There are no save/load
commands.

Available metrics are:

```text
cosine
inner
l2
```

`inner_product` is also accepted as an alias for `inner`.

For cosine and inner product, `--threshold` is a minimum accepted score. For
L2 it is a maximum accepted squared distance. If omitted, the threshold is
unbounded for the selected metric.

## Build

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

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
