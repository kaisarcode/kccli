# kccli

`kccli` contains native command-line projects built on top of precompiled
kclib libraries.

Each CLI lives in its own project directory:

```text
proj/NAME.c/
```

KCCLI builds require `KCLIB_DIR` to point to the root of the kclib monorepo.
For a project `NAME`, public headers are read from
`$KCLIB_DIR/proj/libNAME.c/src/` and precompiled library artifacts are read from
`$KCLIB_DIR/proj/libNAME.c/bin/<arch>/<platform>/`. `kclib/dist` is not used as
a KCCLI build or link input. Library source is not compiled into kccli projects
directly.

## Build

Set `KCLIB_DIR` before building:

```sh
export KCLIB_DIR=/path/to/kclib
```

`KCLIB_DIR` is required. There is no sibling-relative or `kclib/dist` fallback.

Build one project for all configured targets:

```sh
./scripts/build.sh init
```

Build every CLI project:

```sh
./scripts/build.sh all
```

## Tests

Run native and Wine tests for one project:

```sh
./scripts/test.sh init
```

Run tests for every CLI project:

```sh
./scripts/test.sh all
```

Each project also keeps its own direct Make targets such as `make all`,
`make test`, and `make test wine`.

## Distribution

Package all built CLI projects:

```sh
./scripts/dist.sh
```

Distribution artifacts are generated under `dist/` by project, architecture,
and platform.
