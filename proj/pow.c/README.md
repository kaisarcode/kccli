# pow

`pow` creates, solves, and verifies portable proof-of-work challenges.
Challenges and solutions are opaque binary values.

## Quick start

Create a challenge:

```bash
pow challenge 1000000 > challenge.pow
```

Solve it:

```bash
pow solve -i challenge.pow > solution.pow
```

Verify the solution:

```bash
pow verify -c challenge.pow -i solution.pow
```

Successful verification exits with status `0` and produces no normal output.

## Usage

```text
Usage: pow <challenge|solve|verify> [arguments] [options]

Options:
  -i, --input <file>      Read command input from file
  -c, --challenge <file>  Read verify challenge from file
  -h, --help              Show this help
  -v, --version           Show version
```

`challenge <effort>` creates a new challenge. `effort` is the approximate
expected number of nonce attempts, so higher values represent more work.

`solve` reads its challenge from redirected stdin before `--input` when both are
available. `verify` reads the challenge from `--challenge`; its solution input
uses redirected stdin before `--input`.

The CLI does not add transport, encoding, replay, expiry, identity, or admission
policy around the binary proof-of-work values.

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
