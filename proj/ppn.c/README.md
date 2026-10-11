# ppn.c - Native Popup Notification CLI

`ppn` displays one native operating-system notification through `libppn`.

## Usage

```text
Usage: ppn <title> <message>

Options:
  -h, --help     Show this help
  -v, --version  Show version
```

Both `title` and `message` are required and must be non-empty. Quote either
argument when it contains spaces.

Examples:

```sh
ppn "Download complete" "package.tar.gz is ready"
ppn "Build" "Compilation finished successfully"
```

The operating system controls presentation, duration, grouping, and whether a
notification is visible according to the user's notification settings.

## Build

The CLI links against the prebuilt static `libppn` artifacts from `kclib/dist`.

Build all configured targets:

```sh
make all
```

Build one target:

```sh
make x86_64/linux
make x86_64/windows
make x86_64/macos
make aarch64/macos
```

From the `kccli` repository root:

```sh
./scripts/build.sh ppn
```

## Tests

Run native CLI contract tests:

```sh
make test
```

Run the Windows CLI contract tests through Wine:

```sh
make test wine
```

From the `kccli` repository root:

```sh
./scripts/test.sh ppn
```

The CLI contract tests cover argument parsing, help, version reporting, and
invalid invocations. They do not require a visible notification service; the
underlying `libppn` project owns native notification integration testing.

## Platforms

The configured build targets follow `libppn`:

- Linux x86_64
- Windows x86_64
- macOS x86_64
- macOS aarch64

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-147x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
