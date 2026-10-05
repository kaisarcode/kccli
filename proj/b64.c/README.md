# b64

`b64` is a command-line frontend for `libb64.c`.

It encodes binary/text input as RFC 4648 Base64 or decodes strict canonical
Base64 input. Output is written directly to standard output.

## Usage

```text
Usage: b64 -e [text] [options]
       b64 -d [base64] [options]

Options:
  -e, --encode        Encode as Base64
  -d, --decode        Decode Base64
  -i, --input <file>  Read input from file
  -h, --help          Show this help
  -v, --version       Show version
```

Exactly one of `-e`/`--encode` or `-d`/`--decode` is required.

Input precedence is:

```text
argument > stdin > -i/--input
```

A positional argument wins over redirected or piped stdin. Without an argument,
stdin wins over `-i` when stdin contains input. An empty redirected stdin may
fall through to the file.

Encode text from an argument:

```bash
b64 -e "hello world"
```

Output:

```text
aGVsbG8gd29ybGQ=
```

Encode standard input:

```bash
printf 'hello world' | b64 --encode
```

Encode a file:

```bash
b64 --encode -i payload.bin
```

Decode from an argument:

```bash
b64 -d aGVsbG8gd29ybGQ=
```

Decode standard input:

```bash
printf 'aGVsbG8gd29ybGQ=' | b64 --decode
```

Decode a file:

```bash
b64 --decode -i payload.b64
```

Encoding from standard input and `-i` is binary-safe. Decoded bytes are written
raw to standard output. Decoding follows the strict canonical RFC 4648 behavior
of `libb64.c`.

## Build

The CLI links against the precompiled static `libb64.c` artifacts in
`kclib/dist`.

```bash
make
```

Build all configured targets:

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
