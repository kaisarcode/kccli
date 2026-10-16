# Trust workflow

`pact` stores trust state persistently for the current user. Each invocation
opens the same local state automatically.

## Invitation exchange

The inviter creates a one-use invitation:

```bash
pact invite
```

The output contains two tab-separated fields:

```text
<uid><TAB><code>
```

The UID identifies the invited endpoint from the inviter's local state. Transfer
the code to the other endpoint using an application-controlled channel.

The invited endpoint joins with that code:

```bash
pact join '<code>'
```

Its output is also tab-separated:

```text
<uid><TAB><confirmation>
```

That UID identifies the inviter from the joining endpoint's local state. Return
the confirmation to the endpoint that created the invitation.

The inviter completes the relationship:

```bash
pact confirm '<confirmation>'
```

`confirm` prints the invited-endpoint UID originally returned by `invite`.

The two endpoints use different local UIDs for the same relationship. Each side
uses the UID that identifies its remote endpoint.

## Listing trust

List all locally known UIDs:

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

Protect a direct text argument:

```bash
pact seal "$UID" "hello"
```

Protect binary data from standard input:

```bash
cat message.bin | pact seal "$UID" > protected.bin
```

Restore protected data from a file:

```bash
pact unseal "$UID" -i protected.bin > message.bin
```

## Deleting trust

Delete an established relationship or pending invitation with:

```bash
pact "$UID" -d
```

After deletion, that UID is no longer available in the local trust state.

See the [project README](../README.md) for command syntax, input precedence,
build, and test instructions.
