# Direct UDP operation

`punch` is a low-level direct-connectivity interface. It does not discover
public endpoints or exchange peer information on its own.

## Endpoint exchange

Each side supplies the remote endpoint information it already knows and a local
bind preference. In an Internet deployment, usable peer information normally
comes from application signaling, rendezvous, configuration, or another
endpoint-discovery mechanism.

For UDP hole punching, both sides normally need to attempt the path at the same
time and the exchanged peer information must include usable public UDP
endpoints, including the external port.

## Peer candidates

Multiple known endpoints may be supplied for the same peer:

```bash
cat payload.bin | punch send \
    203.0.113.20:5000 \
    192.168.1.20:5000 \
    -p 5000
```

The direct-connectivity attempt evaluates the supplied candidates and uses a
viable path.

## Send behavior

`send` transmits one datagram. Redirected standard input is used first;
otherwise `--input` supplies the datagram from a file. Binary data and empty
datagrams are preserved.

## Receive behavior

`receive` writes exactly one received datagram to standard output. The output is
binary and contains no status or endpoint decoration.

`--receive-timeout 0` waits indefinitely after the direct path has been
established. `--timeout` controls the direct-connectivity attempt.

## Protocol boundaries

`punch` does not provide STUN, rendezvous, endpoint exchange, relaying, identity,
authentication, encryption, reliable delivery, or application session policy.
Those responsibilities belong to the surrounding application or protocol.

See the [project README](../README.md) for command syntax, first use, build, and
test instructions.
