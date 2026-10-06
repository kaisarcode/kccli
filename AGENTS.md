# KCCLI Agent Guide

This repository contains small command-line applications backed by KaisarCode
libraries. The goal of this guide is consistency, not rigidity.

Use the conventions below as the default. Depart from them when the nature of a
CLI genuinely requires it, but keep exceptions deliberate, minimal, and easy to
explain. Do not distort a good CLI design merely to satisfy a superficial rule.

## Core principles

- Keep each CLI small, direct, deterministic, and inspectable.
- Prefer consistency with existing KCCLI projects when several designs are
    equally reasonable.
- Treat the linked kclib public API as the source of truth.
- Do not depend on library internals when the public API can express the task.
- Do not add unrelated dependencies, frameworks, services, generators, or
    runtime layers for convenience.
- Preserve existing behavior unless the task explicitly changes the public CLI
    contract.
- Keep platform-specific behavior only where the underlying library or OS
    requires it.
- Prefer a single obvious way to perform an operation in documentation and
    help output, even if the parser intentionally accepts equivalent forms.

## Repository model

Each CLI normally lives in a self-contained project:

```text
proj/<name>.c/
├── .kcsignore
├── LICENSE
├── Makefile
├── README.md
└── src/
    ├── <name>.c
    └── test.c
```

This two-source-file layout is the default KCCLI structure: one application
source and one test source.

Do not create extra source files, wrappers, entry points, parser shims, helper
translation units, or other layers merely because they make one change easier
to implement. An agent does not have implicit permission to expand the source
tree.

Additional source files are acceptable only when the nature of the application
genuinely requires them and the same result cannot reasonably remain in the
main application source without harming clarity, correctness, portability, or
maintainability. Any such exception should be narrow and justified by the
application itself, not by implementation convenience.

At present, there is no known existing kclib-backed KCCLI that requires an
extra application source file beyond `src/<name>.c`. Prefer keeping that model
unless a future CLI provides a concrete reason not to.

The root scripts discover projects automatically from `proj/<name>.c`. Do not
add a root project registry just to introduce a new CLI.

Typical root commands are:

```sh
./scripts/build.sh <name>
./scripts/test.sh <name>
./scripts/dist.sh <name>
```

Use `all` instead of a project name to operate on every project.

## Relationship with kclib

A KCCLI is normally a thin command-line projection of one public kclib API.
The project Makefile links against prebuilt static artifacts in `kclib/dist`.

The usual relationship is:

```text
proj/<name>.c  ->  kclib/dist/lib<name>.c/<arch>/<platform>/
```

A CLI may add argument parsing, validation, process lifecycle, local socket or
file handling, privilege handling, and other glue that belongs specifically to
the command-line program. It should not reimplement library behavior.

When exposing a library option, prefer a direct and understandable CLI mapping.
The CLI name does not have to copy an awkward internal field name if a shorter
or clearer public name is better, but keep that translation local to the CLI.

## Public command syntax

Design syntax from the user's point of view first.

Prefer positional arguments for the primary identity or object being operated
on when that makes the command immediately readable, for example:

```text
init <id> <cmd>
menu <id> <cmd>
redp2p pub <id>@<index> ...
```

Arguments beginning with `-` or `--` are options, not positional arguments,
regardless of where they appear in the command line. Parsers must separate
options from positional arguments consistently instead of assigning option
tokens to positional slots based on their location.

When an option takes a value, the value is always the following argument:

```text
-p 8080
--port 8080
```

Do not use or accept `--key=value` syntax. The argument following a value-taking
option belongs to that option and must not also be interpreted as a positional
argument. Boolean or action flags such as `--recursive`, `--list`, `--delete`,
`--help`, and `--version` do not take a value.

Prefer named options for configuration values, especially when several values
of the same primitive type would otherwise become ambiguous.

When a commonly used option has a natural short form, use the conventional
short/long pair:

```text
-p, --port <port>
-t, --transport <type>
-d, --delete
-l, --list
-h, --help
-v, --version
```

Do not invent short aliases just to give every option one. Long-only options
are appropriate for less common or more descriptive settings.

When one conceptual setting has multiple possible values, prefer one option
with a value over several mutually exclusive flags when that produces a cleaner
contract. For example, prefer:

```text
-t, --transport <tcp|udp>
```

over separate `--tcp` and `--udp` flags that can conflict.

Boolean flags are still appropriate when they represent an independent action
or state, such as `--delete` or `--list`.

If an option is repeatable, state that explicitly in help or README text.

Avoid duplicated public syntaxes in help. If the parser accepts both prefix and
suffix placement for compatibility or convenience, document one canonical
form unless showing both is necessary to understand distinct behavior.

## Content input convention

For CLIs whose primary operation consumes one block of text or data, prefer the
same three input surfaces when they are natural for that command:

```text
<text-or-data-argument>
stdin
-i, --input <file>
```

Their precedence is:

```text
argument > stdin > -i/--input
```

This means:

- when a positional content argument is present, use it and do not read stdin or
    the file supplied through `-i`;
- otherwise, if stdin is redirected or piped and contains input, use stdin;
- otherwise, if `-i, --input <file>` is present, read input from that file;
- an empty redirected stdin may fall through to `-i` when a file was supplied,
    rather than masking the file with no content.

The positional value is literal content, not a filename. File input should be
spelled explicitly with `-i, --input <file>`.

Use this convention only when the CLI genuinely processes a content payload,
such as text, markup, encoded bytes, or a document. Do not add `-i` to commands
whose natural model is identifiers, configuration, lifecycle operations,
network endpoints, or other non-content inputs merely for superficial
consistency.

Preserve the natural data model of the library. For example, a Base64 encoder
should keep stdin and `-i` binary-safe, while text-processing CLIs may treat
those sources as NUL-terminated text when that matches the public kclib API.

When this convention applies, cover the precedence contract in black-box tests,
including at least the meaningful cases for file input and competing input
sources.

## Defaults

Use sensible defaults when the underlying API has one clear conventional mode.
Document the default where users are likely to care about it.

Do not require an option merely to restate the normal default.

Examples:

```text
-t, --transport <type>     tcp or udp (default: tcp)
--name <name>              Application name (defaults to id)
```

## Help output

Every CLI should provide:

```text
-h, --help
-v, --version
```

These should normally appear last in an `Options` or `Common options` section.

Keep help concise enough to scan in a terminal. It should describe the public
CLI contract, not every implementation detail.

For simple CLIs, a structure like this is preferred:

```text
Usage: <name> ...

Commands:
  ...

Options:
  ...
  -h, --help
  -v, --version
```

When commands accept materially different options, group options by command
instead of mixing everything into one global list:

```text
Usage: <name> <command> [options]

Commands:
  ...

<command-a> options:
  ...

<command-b> options:
  ...

Common options:
  -h, --help
  -v, --version
```

Do not put environment-variable reference documentation in the primary help
when equivalent command-line options exist. Keep the help focused on the CLI
surface and document environment fallbacks or operator configuration in the
project README.

## Environment variables

Environment variables may be supported when useful for automation,
configuration inheritance, secrets, or backwards compatibility.

Whenever practical, important runtime settings should also have a command-line
option. The command-line form is the primary public CLI surface.

If both an environment value and an equivalent command-line option are
supplied, the command-line value should normally take precedence.

Document environment variables and their CLI equivalents in the project
README. Do not make users read source code to discover them.

## Parsing and validation

Reject malformed input early and return a nonzero exit status.

Reject unknown options rather than silently ignoring them.

Validate values close to parsing when possible, for example ports, bounded
integers, enum-like strings, identifiers, and required option arguments.

Prefer clear errors such as:

```text
<name>: missing command
<name>: unknown option: --bad
<name>: --transport requires tcp or udp
```

Do not add complicated parser machinery unless the CLI complexity actually
requires it. Straightforward C argument parsing is preferred for the current
scale of these projects.

## Exit behavior

Use exit status `0` for success and nonzero for invalid usage or operational
failure.

`--help` and `--version` should exit successfully.

Invoking a CLI without enough information may print help, but should normally
return failure if no valid operation was requested.

Keep normal machine-usable output on stdout and diagnostics on stderr.

## Version output

Version output should report the linked library build/version when that library
provides a public version function.

Follow the established project style rather than introducing an independent
KCCLI versioning system without a specific need.

## Source style

Use KaisarCode headers and DocBlocks consistent with the existing repository.
Code must pass `kcs`.

Use C11 and keep builds warning-clean under:

```text
-std=c11 -O2 -Wall -Wextra -Werror
```

Follow the surrounding project's formatting. Do not perform unrelated style
rewrites while changing behavior.

## Makefile pattern

Each project Makefile is responsible for building the CLI directly against the
precompiled static kclib artifacts for its supported targets.

Compile `src/<name>.c` directly as the application source. Do not introduce a
separate `entry.c`, `main.c`, wrapper, source-inclusion shim, or argument
translation layer when the public syntax can be implemented directly in the
application file.

The established pattern includes:

- native target detection,
- Linux cross-build targets,
- Windows cross-build targets,
- macOS cross-build targets where the linked library supports them,
- `make all`,
- `make test`,
- `make test wine` where Windows CLI tests are supported,
- `make clean` as a project target.

Do not assume every library supports every platform. Match the targets and
system libraries that the linked kclib artifact actually supports.

Do not introduce a dynamic-library dependency when the existing KCCLI pattern
uses the static `kclib/dist` artifact.

## Tests

Every CLI project should have `src/test.c` unless there is a strong reason for
a different test arrangement.

The primary test style is black-box CLI contract testing: build the real CLI,
spawn that binary, pass real arguments, and check its exit status and, where
needed, observable output or behavior.

At minimum, test the relevant portions of the public argument contract:

- `--help` succeeds,
- `--version` succeeds,
- unknown options or commands fail,
- required positional arguments are enforced,
- required option values are enforced,
- invalid enum/range values fail,
- deprecated or intentionally removed public syntax fails when appropriate,
- important alternate short/long forms are covered where useful.

When the CLI performs meaningful real operations that can be tested safely and
deterministically, add integration coverage using the actual CLI and actual
library path rather than replacing it with an in-process imitation.

Tests should remain self-contained. Do not require external public services,
network infrastructure, or unrelated third-party daemons for ordinary contract
or unit-style testing.

Use the existing result format:

```text
[1/N] [PASS] name: detail
...

N passed, 0 failed
```

The project Makefile should define `KC_CLI_PATH` for the test binary so the same
`src/test.c` can exercise the native CLI and, where supported, the Windows CLI
through Wine.

## README expectations

Each project README should explain:

- what the CLI does,
- how to build it,
- how to test it,
- the canonical command syntax,
- the accepted options,
- concise examples,
- environment compatibility when applicable,
- any platform or runtime behavior that users need to know.

Keep README syntax synchronized with the actual parser and `--help` output.
When changing a public option, update source, tests, help, and README together.

## Creating a new KCCLI

When adding a new CLI, first inspect the most similar existing project rather
than designing it from scratch.

Normally:

1. Create `proj/<name>.c/` using an existing project with similar platform and
    library requirements as the structural reference.
2. Add `src/<name>.c` as the CLI implementation.
3. Add `src/test.c` with black-box contract tests.
4. Add a Makefile that links the appropriate static library from `kclib/dist`
    and exposes the standard build/test targets applicable to that library.
5. Add `README.md`, `LICENSE`, and `.kcsignore` following repository patterns.
6. Verify that the root `scripts/build.sh` and `scripts/test.sh` discover the
    project automatically; do not edit them merely to enumerate the new name.
7. Run the project build and tests and then run `kcs` from the repository root.

Do not create additional source files during project setup unless the CLI has a
concrete architectural need that cannot reasonably be handled by
`src/<name>.c`. Convenience, parser organization, argument normalization, or a
preference for extra layering are not sufficient reasons.

Do not copy platform link flags blindly. Derive them from the library and the
closest working KCCLI project.

## Changing an existing CLI

Keep changes narrow.

If changing public syntax, update all affected surfaces in the same change:

```text
parser
help
contract tests
README
```

Do not leave old syntax documented after removing it, and do not document new
syntax that the parser does not accept.

When an old form is intentionally removed, reject it explicitly when doing so
produces a clearer diagnostic and protects the new contract.

Do not add a new source file as a workaround for changing an existing parser or
public surface. Modify the existing application source directly unless the
application itself has developed a real architectural need for separation.

Avoid changing the linked kclib solely to make the CLI implementation easier.
If the public library API is genuinely insufficient, treat that as a separate
library design decision rather than quietly coupling the CLI to internals.

## Consistency rule

When two or more designs are functionally equivalent, prefer the one already
used elsewhere in KCCLI.

When consistency conflicts with clarity, correctness, portability, or the
natural model of the underlying library, choose the better CLI design and keep
the exception small.

This guide defines defaults, not an unbreakable protocol. The objective is that
new KCCLI tools feel like members of the same family without forcing unrelated
commands into identical shapes.
