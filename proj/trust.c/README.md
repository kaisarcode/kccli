# trust

`trust` establishes persistent trust relationships between endpoints and protects
binary messages exchanged between them.

## Usage

```text
Usage: trust <command> [arguments] [options]

Commands:
  invite                    Create a one-use invitation
  join <code>               Join an invitation
  confirm <confirmation>    Confirm a joined invitation
  seal <uid> [message]      Protect data for a remote UID
  unseal <uid> [data]       Restore data addressed to a local UID
  revoke <uid>              Revoke a relationship or invitation

Options:
  -i, --input <file>        Read seal/unseal input from file
  -h, --help                Show this help
  -v, --version             Show version
```

Trust state is stored persistently for the current user. Each invocation opens
the same local state automatically.

## Establishing trust

Create an invitation:

```bash
trust invite
```

The output has two tab-separated fields:

```text
<uid><TAB><code>
```

The UID identifies the invited endpoint from the inviter's side. Send the code
to the other endpoint using any transport you choose.

The invited endpoint joins with that code:

```bash
trust join '<code>'
```

The output is also tab-separated:

```text
<uid><TAB><confirmation>
```

This UID identifies the inviter from the joining endpoint's side. Send the
confirmation back to the endpoint that created the invitation.

The inviter completes the relationship:

```bash
trust confirm '<confirmation>'
```

`confirm` prints the same invited-endpoint UID originally returned by
`trust invite`.

The two endpoints use different UIDs for the same relationship. Each endpoint
uses the UID that identifies the remote endpoint when sealing data.

## Protecting data

Protect a text argument:

```bash
trust seal "$UID" "hello"
```

Protect binary data from standard input:

```bash
cat message.bin | trust seal "$UID" > protected.bin
```

Or read it from a file:

```bash
trust seal "$UID" -i message.bin > protected.bin
```

Restore protected data:

```bash
trust unseal "$UID" -i protected.bin > message.bin
```

For `seal` and `unseal`, input is selected in this order:

```text
argument > stdin > -i/--input
```

Standard input, file input, and command output are binary-safe.

## Revoking trust

Revoke an established relationship or a pending invitation:

```bash
trust revoke "$UID"
```

After revocation, that UID can no longer be used locally for the removed trust
relationship.

## Build

```bash
make
```

Build all configured desktop targets:

```bash
make all
```

## Tests

```bash
make test
```

Windows tests can be run through Wine:

```bash
make test wine
```

## License

GNU General Public License version 3 or later.
