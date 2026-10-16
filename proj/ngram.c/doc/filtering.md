# Filtering traversal

`ngram --filter` runs one command for each visited chunk and uses whether that
command produced standard output as a traversal decision.

## Filter input

For every visited chunk, `ngram` runs the configured command synchronously. The
chunk bytes are written to the command's standard input followed by a newline.

The command's exit status does not select or reject the chunk.

## Filter decision

If the filter writes anything to standard output, the current span is closed.
Smaller windows fully contained in that span are skipped.

If the filter writes nothing to standard output, traversal continues normally
through smaller windows in that span.

The filter's output is consumed only as this decision signal. It is not copied
to `ngram` standard output.

## Current chunk output

`ngram` prints the current chunk before applying the filter decision. The filter
therefore controls later traversal; it does not remove the current chunk from
normal output.

For example, close a span whenever `grep` sees `brown`:

```bash
ngram "the quick brown fox jumps" --filter "grep brown"
```

Without `--filter`, every candidate chunk in the configured traversal range is
printed.

See the [project README](../README.md) for input precedence, window options,
build, and test instructions.
