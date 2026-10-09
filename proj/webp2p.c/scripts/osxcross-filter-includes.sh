#!/bin/bash
# Filters host-only include paths injected by libjuice during OSXCross builds.
# libjuice adds /usr/local/include for native Apple builds; under OSXCross that
# path belongs to the Linux host and must not reach the cross compiler.

set -e

compiler="$1"
shift

case "$compiler" in
    *osxcross*|*o64-clang*|*oa64-clang*)
        args=()
        skip_next=0
        for arg in "$@"; do
            if [ "$skip_next" -eq 1 ]; then
                skip_next=0
                if [ "$arg" = "/usr/local/include" ]; then
                    continue
                fi
                args+=("-I" "$arg")
                continue
            fi
            case "$arg" in
                -I/usr/local/include)
                    continue
                    ;;
                -I)
                    skip_next=1
                    continue
                    ;;
            esac
            args+=("$arg")
        done
        if [ "$skip_next" -eq 1 ]; then
            args+=("-I")
        fi
        exec "$compiler" "${args[@]}"
        ;;
    *)
        exec "$compiler" "$@"
        ;;
esac
