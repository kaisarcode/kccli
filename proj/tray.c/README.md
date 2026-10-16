# tray

`tray` creates one persistent native system-tray icon with an ordered menu.
Menu items can run commands and an optional quit item closes the tray.

## Quick start

```bash
tray \
    --tooltip "My application" \
    --item "Open site" "xdg-open https://example.com" \
    --separator \
    --quit "Quit"
```

Items and separators are created in the same order in which their options are
provided.

## Usage

```text
Usage: tray [options]

Options:
  --icon <icon>            Icon file path or platform icon name
  --tooltip <text>         Tray tooltip
  --item <text> <command>  Add a menu item; repeatable
  --separator              Add a menu separator; repeatable
  --quit <text>            Add an item that closes the tray
  -h, --help               Show this help
  -v, --version            Show version
```

`--item` runs its command when activated and leaves the tray open. `--quit`
adds an item that ends the tray process cleanly. `Ctrl+C` and termination
signals also close the tray.

## Platform notes

Windows expects `.ico` file paths. Linux accepts icon file paths or icon names.
macOS accepts icon file paths or named AppKit images.

Configured targets are:

```text
x86_64/linux
x86_64/windows
x86_64/macos
aarch64/macos
```

Linux builds require GTK 3 development files.

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
