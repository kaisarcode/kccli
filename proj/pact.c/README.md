# pact

`pact` establishes persistent trusted relationships between endpoints and
protects binary messages exchanged through those relationships.

## Quick start

Create a one-use invitation:

```bash
pact invite
```

Join that invitation on the other endpoint:

```bash
pact join '<code>'
```

Return the confirmation to the inviter and complete the relationship:

```bash
pact confirm '<confirmation>'
```

The handshake returns endpoint-local UIDs that are then used with `seal` and
`unseal`.

## Usage

```text
Usage: pact <command> [arguments] [options]

Commands:
  invite                    Create a one-use invitation
  join <code>               Join an invitation
  confirm <confirmation>    Confirm a joined invitation
  seal <uid> [message]      Protect data for a remote UID
  unseal <uid> [data]       Restore data addressed to a local UID
  -l [uid]                  List relationships and invitations
  <uid> -d                  Delete a relationship or invitation

Options:
  -l, --list                List relationships and invitations
  -d, --delete              Delete a relationship or invitation
  -i, --input <file>        Read seal/unseal input from file
  -h, --help                Show this help
  -v, --version             Show version
```

## Data input

For `seal` and `unseal`, input is selected in this order:

```text
argument > stdin > -i/--input
```

Standard input, file input, and command output are binary-safe.

See [Trust workflow](doc/trust-workflow.md) for invitation exchange, endpoint
UIDs, listing, deletion, and protected-data examples.

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
