# pact

`pact` establishes persistent trusted relationships between endpoints and
protects binary messages exchanged between them.

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

Trust state is stored persistently for the current user. Each invocation opens
the same local state automatically.

## Establishing trust

Create an invitation:

```bash
pact invite
```

The output has two tab-separated fields:

```text
<uid><TAB><code>
```

The UID identifies the invited endpoint from the inviter's side. Send the code
to the other endpoint using any transport you choose.

The invited endpoint joins with that code:

```bash
pact join '<code>'
```

The output is also tab-separated:

```text
<uid><TAB><confirmation>
```

This UID identifies the inviter from the joining endpoint's side. Send the
confirmation back to the endpoint that created the invitation.

The inviter completes the relationship:

```bash
pact confirm '<confirmation>'
```

`confirm` prints the same invited-endpoint UID originally returned by
`pact invite`.

The two endpoints use different UIDs for the same relationship. Each endpoint
uses the UID that identifies the remote endpoint when sealing data.

## Listing trust

List all locally known remote UIDs:

```bash
pact -l
```

List one UID:

```bash
pact -l "$UID"
```

Each row has three tab-separated fields:

```text
<uid><TAB><status><TAB><created>
```

`status` is `pending` for an unconfirmed invitation and `confirmed` for an
established relationship. `created` is the Unix timestamp of the persisted
state record.

## Protecting data

Protect a text argument:

```bash
pact seal "$UID" "hello"
```

Protect binary data from standard input:

```bash
cat message.bin | pact seal "$UID" > protected.bin
```

Or read it from a file:

```bash
pact seal "$UID" -i message.bin > protected.bin
```

Restore protected data:

```bash
pact unseal "$UID" -i protected.bin > message.bin
```

For `seal` and `unseal`, input is selected in this order:

```text
argument > stdin > -i/--input
```

Standard input, file input, and command output are binary-safe.

## Deleting trust

Delete an established relationship or a pending invitation:

```bash
pact "$UID" -d
```

After deletion, that UID can no longer be used locally for the removed trust
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

---

## License

[![LGPLv3](https://www.gnu.org/graphics/lgplv3-127x51.png)](https://www.gnu.org/licenses/lgpl-3.0.html)

This project is distributed under the **GNU Lesser General Public License version 3 (LGPLv3)**.
