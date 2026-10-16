# Native bridge

`wvw --exec` exposes selected native executables to trusted WebView content
through `NativeBridge.exec`.

## Executable registration

Register each allowed executable path with a separate `--exec` option:

```bash
wvw ./index.html \
    --exec ./myexe \
    --exec ./other
```

Page code addresses a registered executable by its basename:

```js
NativeBridge.exec(["myexe", "--param", "hello"]);
```

Only explicitly registered executables are available through this CLI surface.
Do not expose a general shell or executable set broader than the page requires.

## Trust boundary

Treat page content and bridge parameters as application input. Exposing an
executable gives that content a native capability, so the allowed set should be
small and intentional.

The CLI does not provide a command-line option for arbitrary application bridge
methods. Its native exposure surface is the executable allowlist supplied with
`--exec`.

## Window behavior

`--hidden` controls initial visibility. `--unlist` removes the window from the
platform's normal window list without making it equivalent to a hidden window.

`--always-on-top`, `--click-through`, and `--no-focus` apply native window
behavior at startup. `--borderless` and transparent backgrounds are useful for
overlay-style windows.

See the [project README](../README.md) for the full CLI syntax, platform
requirements, build, and test instructions.
