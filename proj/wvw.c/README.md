# wvw.c - Native WebView CLI

`wvw` opens one native WebView window using `libwvw`.

The URL is the primary positional argument. Window configuration is exposed as
command-line options and maps directly to `kc_wvw_options_t`.

## Build

Build the native host target:

```sh
make
```

Build every target supported by `libwvw`:

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

Run the native CLI contract tests:

```sh
make test
```

Run the x86_64 Windows contract tests through Wine:

```sh
make test wine
```

The contract tests validate argument handling without requiring a real WebView
to be opened.

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
-h, --help              Show help
-v, --version           Show version
```

The process remains alive while the WebView instance is active and closes the
window cleanly on SIGINT or SIGTERM.

## Transparency

`libwvw` uses the alpha byte of an eight-digit background color. Use `00` alpha
for a fully transparent background.

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

## Library mapping

The CLI maps its startup options directly to these public `libwvw` fields:

```text
url             -> options.url
title           -> options.title
background      -> options.background
icon            -> options.icon
width           -> options.width
height          -> options.height
x               -> options.posx
y               -> options.posy
fullscreen      -> options.fullscreen
borderless      -> options.borderless
always-on-top   -> options.always_on_top
click-through   -> options.click_through
no-focus        -> options.no_focus
hidden          -> options.hidden
unlist          -> options.unlist
```

Runtime library operations such as navigate, show/hide, minimize/maximize,
setters/getters, bridge callbacks, and bridge events act on an already-open
`kc_wvw_t` handle. They are library APIs rather than startup configuration, so
they are not represented as artificial one-shot CLI flags.

---

## License

This project is distributed under the GNU General Public License version 3.
