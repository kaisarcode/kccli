# Template syntax

`tpl` supports escaped and raw expressions, includes, local variables, named
blocks, conditionals, and foreach loops.

## Output expressions

Escaped output uses double braces:

```text
{{ title }}
```

Raw output uses triple braces:

```text
{{{ html }}}
```

Dot notation can address nested-style keys and foreach aliases:

```text
{{ user.name }}
```

Template comments are removed from output:

```text
{{/* internal template note */}}
```

## Includes

Include another template relative to the configured root:

```text
{{@include "partial.html"}}
```

The root defaults to the current directory and can be changed with `--root`.

## Variables

Set a value in the current template scope:

```text
{{@var name expr}}
```

CLI variables are provided with repeatable `--var <key> <value>` options before
rendering begins.

## Blocks

Define a named block:

```text
{{@setblock card}}
<div>{{ title }}</div>
{{@endsetblock}}
```

Render it later:

```text
{{@block card}}
```

Inline block properties can be supplied when rendering:

```text
{{@block card [ title: "Hello" ]}}
```

## Conditionals

Conditional rendering uses `if`, optional `else`, and `endif`:

```text
{{@if logged_in}}
Welcome
{{@else}}
Sign in
{{@endif}}
```

Expressions support negation, equality, inequality, logical AND, and logical OR.

## Iteration

Iterate over a list with `foreach`:

```text
{{@foreach item in items}}
{{ item }}
{{@endforeach}}
```

The loop alias is scoped to the iteration.

See the [project README](../README.md) for CLI syntax, input precedence, build,
and test instructions.
