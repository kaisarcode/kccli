# hnsw

`hnsw` builds one in-memory HNSW index from a vector dataset, runs one query,
prints the matches, and exits.

## Quick start

```bash
printf 'a\t1,0\nb\t0,1\n' | hnsw -q 1,0 -d 2
```

Results are written as headerless CSV:

```text
<id>,<score>
```

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

## Dataset and metrics

Each input row contains an id, one tab, and a comma-separated vector:

```text
<id><TAB><vector>
```

The positional dataset argument is literal content. Input is selected in this
order:

```text
argument > stdin > -i/--input
```

Metrics are `cosine`, `inner`, and `l2`; `inner_product` is also accepted as an
alias for `inner`. For cosine and inner product, `--threshold` is a minimum
accepted score. For L2, it is a maximum accepted squared distance.

The index exists only for the current invocation.

## Build

The Makefile requires `KCLIB_DIR` to point to the checkout that contains the
matching prebuilt library and public header.

```bash
export KCLIB_DIR=path/to/kclib
make
make test
```

Build every configured target with:

```bash
make all
```

`make all` requires the toolchains for the targets being built. Run the x86_64
Windows CLI tests through Wine with:

```bash
make test wine
```

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
