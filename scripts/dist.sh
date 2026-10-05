#!/bin/bash
# kccli dist tool
# Summary: Packages one or all built CLI projects by platform and architecture.
# Author:  KaisarCode
# Website: https://kaisarcode.com
# License: GNU General Public License v3.0

set -e

# Prints command usage information.
# @return 0 on success.
usage() {
    echo "Usage: $0 all|NAME" >&2
}

# Resolves one logical kccli name to its project directory.
# @param proj_dir Projects directory.
# @param name Logical kccli name.
# @return 0 on success, 1 when the project does not exist.
resolve_project() {
    local proj_dir="$1"
    local name="$2"

    if [ -d "$proj_dir/$name.c" ]; then
        printf '%s\n' "$proj_dir/$name.c"
        return 0
    fi

    return 1
}

# Computes the SHA-256 digest of one file.
# @param file File path.
# @return 0 on success; the digest is written to stdout.
compute_sha256() {
    local file="$1"

    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$file" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$file" | awk '{print $1}'
    else
        openssl dgst -sha256 "$file" | awk '{print $NF}'
    fi
}

# Returns the logical CLI name for a NAME.c project.
# @param dir Project directory.
# @return 0 on success; the CLI name is written to stdout.
project_name() {
    local dir="$1"
    local name

    name=$(basename "$dir")
    printf '%s\n' "${name%.c}"
}

# Packages all built targets for one CLI project.
# @param project_dir Project directory.
# @return 0 on success.
package_project() {
    local project_dir="$1"
    local name bin_dir project_dist arch_dir arch platform_dir platform
    local executable package_name package work sidecar sidecar_name

    name=$(project_name "$project_dir")
    bin_dir="$project_dir/bin"
    [ -d "$bin_dir" ] || return 0

    project_dist="$DIST_DIR/$(basename "$project_dir")"
    rm -rf "$project_dist"
    mkdir -p "$project_dist"

    echo "Processing $(basename "$project_dir")..."

    for arch_dir in "$bin_dir"/*; do
        [ -d "$arch_dir" ] || continue
        arch=$(basename "$arch_dir")

        for platform_dir in "$arch_dir"/*; do
            [ -d "$platform_dir" ] || continue
            platform=$(basename "$platform_dir")

            case "$platform" in
                windows) executable="$name.exe" ;;
                *) executable="$name" ;;
            esac

            [ -f "$platform_dir/$executable" ] || continue

            package_name="$name-$platform-$arch.zip"
            package="$project_dist/$package_name"
            work=$(mktemp -d)

            cp "$platform_dir/$executable" "$work/$executable"
            for sidecar in "$platform_dir"/*; do
                [ -f "$sidecar" ] || continue
                sidecar_name=$(basename "$sidecar")
                [ "$sidecar_name" = "$executable" ] && continue
                cp "$sidecar" "$work/$sidecar_name"
            done
            [ -f "$project_dir/README.md" ] &&
                cp "$project_dir/README.md" "$work/README.md"
            [ -f "$project_dir/LICENSE" ] &&
                cp "$project_dir/LICENSE" "$work/LICENSE"

            (
                cd "$work"
                zip -qr "$package" .
            )
            rm -rf "$work"
        done
    done

    : > "$project_dist/SHA256SUMS"
    for package in "$project_dist"/*.zip; do
        [ -f "$package" ] || continue
        printf '%s  %s\n' \
            "$(compute_sha256 "$package")" \
            "$(basename "$package")" >> "$project_dist/SHA256SUMS"
    done
}

# Dispatches one project distribution or all project distributions.
# @param target Logical kccli name or all.
# @return 0 on success.
main() {
    local script_dir root_dir proj_dir target project_dir

    [ "$#" -eq 1 ] || {
        usage
        exit 1
    }

    script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
    root_dir=$(dirname "$script_dir")
    proj_dir="$root_dir/proj"
    target="$1"

    [ -d "$proj_dir" ] || {
        echo "error: projects directory not found: $proj_dir" >&2
        exit 1
    }

    DIST_DIR="$root_dir/dist"
    mkdir -p "$DIST_DIR"

    if [ "$target" = "all" ]; then
        rm -rf "$DIST_DIR"
        mkdir -p "$DIST_DIR"

        for project_dir in "$proj_dir"/*.c; do
            [ -d "$project_dir" ] || continue
            package_project "$project_dir"
        done

        echo "Done. Packages ready in $DIST_DIR/"
        return 0
    fi

    case "$target" in
        *[!A-Za-z0-9_-]*|'')
            echo "error: invalid project name: $target" >&2
            usage
            exit 1
            ;;
    esac

    project_dir=$(resolve_project "$proj_dir" "$target") || {
        echo "error: project not found: $target" >&2
        exit 1
    }

    package_project "$project_dir"
    echo "Done. Packages ready in $DIST_DIR/$(basename "$project_dir")/"
}

main "$@"
