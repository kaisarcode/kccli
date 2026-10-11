# wvw.c - Native WebView CLI

`wvw` opens one native WebView window.

The URL is the primary positional argument. Window configuration is provided
through command-line options.

## Build

Build the native host target:

```sh
make
```

Build all configured targets:

```sh
make all
```

Configured targets are:

```text
x86_64/linux
x86_64/windows
x86_64/macos
aarch64/macos
```

Linux builds require GTK 3 and WebKitGTK 4.1 development files. Windows uses
WebView2 at runtime. macOS uses WebKit/Cocoa/Foundation.

## Tests

Run the native CLI tests:

```sh
make test
```

Run the x86_64 Windows tests through Wine:

```sh
make test wine
```

The tests validate argument handling without opening a real WebView.

## CLI

```text
wvw <url> [options]
```

Options:

```text
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
--unlist                Exclude the window from the platform window list
--exec <path>           Expose executable to NativeBridge.exec
-h, --help              Show help
-v, --version           Show version
```

The process remains alive while the WebView is active and closes the window on
SIGINT or SIGTERM.

## Transparency

Use the alpha byte of an eight-digit background color. `00` is fully
transparent.

For example:

```bash
wvw https://example.com --background 00000000 --borderless
```

A semi-transparent black background can use:

```bash
wvw https://example.com --background 80000000
```

Six-digit colors are opaque RGB values:

```bash
wvw https://example.com --background 202020
```

## Examples

Open a normal window:

```bash
wvw https://example.com
```

Set title and size:

```bash
wvw https://example.com \
    --title "Example" \
    --width 1024 \
    --height 768
```

Set an explicit position:

```bash
wvw https://example.com --x 100 --y 80
```

Create a borderless transparent always-on-top window:

```bash
wvw https://example.com \
    --background 00000000 \
    --borderless \
    --always-on-top
```

Create a click-through overlay that does not take focus and is not listed:

```bash
wvw https://example.com \
    --background 00000000 \
    --borderless \
    --always-on-top \
    --click-through \
    --no-focus \
    --unlist
```

Start fullscreen:

```bash
wvw https://example.com --fullscreen
```

Start hidden:

```bash
wvw https://example.com --hidden
```

Expose only selected native executables to page code:

```bash
wvw ./index.html \
    --exec /path/to/myexe \
    --exec /path/to/other
```

```js
NativeBridge.exec(["myexe", "--param", "hello"]);
```

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-127x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
