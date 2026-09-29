# kccli

`kccli` contains native command-line projects built on top of precompiled
kclib static libraries.

Each CLI lives in its own project directory:

```text
proj/NAME.c/
```

Projects build against kclib artifacts under
`kclib/dist/libNAME.c/<arch>/<platform>/` when applicable. The library source
is not compiled into kccli projects directly.

## Build

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
