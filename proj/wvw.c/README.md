# wvw

`wvw` opens one native WebView window for a URL or local page and keeps the
process alive while the window is active.

## Quick start

```bash
wvw https://example.com
```

Set the title and content size:

```bash
wvw https://example.com --title "Example" --width 1024 --height 768
```

## Usage

```text
Usage: wvw <url> [options]

Options:
  --title <title>         Window title (default: wvw)
  --background <color>    RRGGBB or AARRGGBB; 00 alpha is transparent
  --icon <icon>           Icon file path or platform icon name
  --width <px>            Content width (default: 1280)
  --height <px>           Content height (default: 720)
  --x <px>                Window X position
  --y <px>                Window Y position
  --fullscreen            Start fullscreen
  --borderless            Start without native window borders
  --always-on-top         Keep the window above normal windows
  --click-through         Pass pointer input through the window
  --no-focus              Prevent the window from taking focus
  --hidden                Start with the native window hidden
  --unlist                Exclude the window from the window list
  --exec <path>           Expose executable to NativeBridge.exec
  -h, --help              Show this help
  -v, --version           Show version
```

Eight-digit backgrounds use `AARRGGBB`; alpha `00` is fully transparent. Six
hexadecimal digits are opaque RGB.

`--exec` is repeatable. See [Native bridge](doc/native-bridge.md) before exposing
native executables to page content.

`Ctrl+C` and termination signals close the WebView cleanly.

## Platform requirements

Configured targets are:

```text
x86_64/linux
x86_64/windows
x86_64/macos
aarch64/macos
```

Linux builds require GTK 3 and WebKitGTK 4.1 development files. Windows uses
WebView2 and the build packages `WebView2Loader.dll` beside the executable.
macOS uses WebKit and Cocoa.

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
