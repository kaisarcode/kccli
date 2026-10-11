# tray CLI

`tray` creates one persistent native system tray icon and an ordered menu.

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

Items and separators are created in the same order in which their options are
provided.

`--item` executes its command when activated. The tray remains open after the
command completes.

`--quit` adds a menu item whose activation ends the tray process cleanly.

`Ctrl+C` and process termination signals also close the tray cleanly.

## Examples

Create a tray with one command and a quit item:

```bash
tray \
    --tooltip "My application" \
    --item "Open site" "xdg-open https://example.com" \
    --separator \
    --quit "Quit"
```

With an icon:

```bash
tray --icon ./app.png --tooltip "App" --quit "Exit"
```

Platform icon behavior:

- Windows expects `.ico` file paths.
- Linux accepts icon file paths or icon names.
- macOS accepts icon file paths or named AppKit images.

## Build

```bash
make
```

Build all supported targets:

```bash
make all
```

Configured targets:

```text
x86_64/linux
x86_64/windows
x86_64/macos
aarch64/macos
```

Linux builds require GTK 3 development files. Windows uses the native system
tray APIs. macOS uses Cocoa/AppKit and services the AppKit event loop on the
main thread while the tray is active.

## Tests

```bash
make test
```

The tests validate the argument contract without requiring an interactive tray
session.

Windows parser tests can be run through Wine:

```bash
make test wine
```

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-127x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
