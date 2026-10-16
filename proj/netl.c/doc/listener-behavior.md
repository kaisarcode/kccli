# Listener behavior

`netl` keeps named listeners resident after registration and maps incoming
network traffic to local commands.

## TCP dispatch

For TCP, each accepted connection is associated with one command process.
Bytes received from the peer are written to that process on standard input.
Bytes written by the command to standard output are sent back to the same peer.

The connection remains associated with that command process until the session
ends.

## UDP dispatch

For UDP, each received datagram starts one command process. The datagram bytes
are written to that process on standard input.

UDP dispatch is datagram-oriented. A command process is not kept as a persistent
session for later datagrams.

## Registration lifecycle

Registering an existing id replaces the named listener. Deleting a listener
stops the resident process and removes its registration.

The registering command does not stay attached to the terminal after successful
creation.

## Listing

`netl -l` lists every listener and `netl -l <id>` selects one id. Both forms use
this tab-separated schema:

```text
<id><TAB><port><TAB><transport><TAB><cmd>
```

The command field is the command string stored for the listener.

## Runtime directory

Set `KC_NETL_DIR` to override the runtime state directory. This is useful for
isolated tests, containers, or applications that need a controlled state path.

See the [project README](../README.md) for command syntax, first use, build, and
test instructions.
