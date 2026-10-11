# demo

`demo` is the command-line interface for `libdemo.c`. It creates a greeting for
one name.

## Usage

```text
Usage: demo <name> [options]

Options:
  -h, --help     Show this help
  -v, --version  Show version
```

For example:

```bash
demo John
```

prints:

```text
Hello John!
```

Quote names containing spaces:

```bash
demo "John Doe"
```

## Build

```bash
make
```

Build all configured targets:

```bash
make all
```

Android targets use the same NDK baseline as `kclib`:

```bash
make aarch64/android
make armv7/android
```

The resulting executables are written to:

```text
bin/aarch64/android/demo
bin/armv7/android/demo
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

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-127x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
