# pow

`pow` creates, solves, and verifies portable proof-of-work challenges backed by `libpow.c`.

## Usage

```text
Usage: pow <command> [arguments] [options]

Commands:
  challenge <effort>         Create a proof-of-work challenge
  solve                      Solve a challenge
  verify                     Verify a solution against a challenge

solve options:
  -i, --input <file>         Read challenge from file

verify options:
  -c, --challenge <file>     Read challenge from file
  -i, --input <file>         Read solution from file

Common options:
  -h, --help                 Show this help
  -v, --version              Show version
```

Challenges and solutions are opaque binary entities. The CLI does not add transport, encoding, replay, expiry, identity, or admission semantics.

Create a challenge:

```bash
pow challenge 1000000 > challenge.pow
```

Solve it:

```bash
pow solve -i challenge.pow > solution.pow
```

or:

```bash
pow solve < challenge.pow > solution.pow
```

Verify it:

```bash
pow verify -c challenge.pow -i solution.pow
```

or:

```bash
cat solution.pow | pow verify -c challenge.pow
```

Successful verification exits with status `0` and produces no normal output. Invalid proofs and operational failures return a nonzero status.

`effort` is the approximate expected number of nonce attempts. Higher values represent harder challenges.

## Build

```bash
make
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

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
