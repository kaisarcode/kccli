#!/bin/bash
# kccli dist tool
# Summary: Packages CLI artifacts into dist with checksums and a manifest.
# Author:  KaisarCode
# Website: https://kaisarcode.com
# License: GNU Lesser General Public License v3.0

set -e

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(dirname "$SCRIPT_DIR")
PROJ_DIR="$ROOT_DIR/proj"
DIST_DIR="$ROOT_DIR/dist"
declare -A MANIFEST_SOURCE_SHA=()
declare -A MANIFEST_SOURCE_COUNT=()
declare -A MANIFEST_PACKAGE_SHA=()
declare -A MANIFEST_PACKAGE_SIZE=()

# Prints command usage information.
# @return 0 on success.
usage() {
    echo "Usage: $0 all|NAME" >&2
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

# Checks whether a path is a current kccli project.
# @param project_path Project directory path.
# @return 0 when current, 1 otherwise.
is_project() {
    local project_path="$1"

    [ -d "$project_path" ] || return 1
    [ -f "$project_path/Makefile" ] || return 1
    return 0
}

# Resolves one logical kccli name to its project directory.
# @param proj_dir Projects directory.
# @param name Logical kccli name.
# @return 0 on success, 1 when the project does not exist.
resolve_project() {
    local proj_dir="$1"
    local name="$2"
    local project_dir

    project_dir="$proj_dir/$name.c"
    if is_project "$project_dir"; then
        printf '%s\n' "$project_dir"
        return 0
    fi

    return 1
}

# Computes the SHA-256 digest of one newly created package.
# @param file Package path.
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

# Loads build and package checksums from the current distribution manifest.
# @return 0 on success.
load_manifest_state() {
    local manifest kind path sha prefix size

    MANIFEST_SOURCE_SHA=()
    MANIFEST_SOURCE_COUNT=()
    MANIFEST_PACKAGE_SHA=()
    MANIFEST_PACKAGE_SIZE=()
    manifest="$DIST_DIR/manifest.json"
    [ -f "$manifest" ] || return 0

    while IFS=$'\t' read -r kind path sha size; do
        [ -n "$kind" ] || continue
        [ -n "$path" ] || continue

        case "$kind" in
            S)
                MANIFEST_SOURCE_SHA["$path"]="$sha"
                prefix="${path%/*}"
                MANIFEST_SOURCE_COUNT["$prefix"]=$((
                    ${MANIFEST_SOURCE_COUNT["$prefix"]:-0} + 1
                ))
                ;;
            P)
                MANIFEST_PACKAGE_SHA["$path"]="$sha"
                MANIFEST_PACKAGE_SIZE["$path"]="$size"
                ;;
        esac
    done < <(
        awk '
            /"sources": \{/ {
                in_sources = 1
                next
            }
            in_sources && /^[[:space:]]*},?[[:space:]]*$/ {
                in_sources = 0
                next
            }
            in_sources {
                path = $0
                sha = $0
                sub(/^[[:space:]]*"/, "", path)
                sub(/": ".*$/, "", path)
                sub(/^.*": "/, "", sha)
                sub(/".*$/, "", sha)
                if (path != "" && sha != "") {
                    printf "S\t%s\t%s\t\n", path, sha
                }
                next
            }
            /"size_bytes":/ {
                size = $0
                sub(/^.*"size_bytes": /, "", size)
                sub(/,.*/, "", size)
            }
            /"sha256":/ {
                sha = $0
                sub(/^.*"sha256": "/, "", sha)
                sub(/".*$/, "", sha)
            }
            /"path":/ {
                path = $0
                sub(/^.*"path": "/, "", path)
                sub(/".*$/, "", path)
                if (sha != "" && path != "") {
                    printf "P\t%s\t%s\t%s\n", path, sha, size
                }
                sha = ""
                size = ""
            }
        ' "$manifest"
    )
}

# Removes checksum files left by the superseded dist layout.
# @return 0 on success.
remove_dist_checksum_files() {
    [ -d "$DIST_DIR" ] || return 0
    find "$DIST_DIR" -type f -name SHA256SUMS -delete
}

# Removes one target's build state from the loaded manifest data.
# @param prefix Project/architecture/platform prefix.
# @return 0 on success.
clear_target_source_state() {
    local prefix="$1"
    local key

    for key in "${!MANIFEST_SOURCE_SHA[@]}"; do
        case "$key" in
            "$prefix"/*)
                unset 'MANIFEST_SOURCE_SHA[$key]'
                ;;
        esac
    done
    unset 'MANIFEST_SOURCE_COUNT[$prefix]'
}

# Records one compiled target's build checksums in manifest state.
# @param project_dir Project directory.
# @param target_dir Compiled target directory.
# @return 0 on success.
record_target_source_state() {
    local project_dir="$1"
    local target_dir="$2"
    local project_base relative prefix checksum sha file count

    project_dir="${project_dir%/}"
    project_base=$(basename "$project_dir")
    relative="${target_dir#"$project_dir/bin/"}"
    prefix="$project_base/$relative"
    checksum="$target_dir/SHA256SUMS"

    clear_target_source_state "$prefix"
    count=0
    while read -r sha file; do
        [ -n "$sha" ] || continue
        [ -n "$file" ] || continue
        MANIFEST_SOURCE_SHA["$prefix/$file"]="$sha"
        count=$((count + 1))
    done < "$checksum"
    MANIFEST_SOURCE_COUNT["$prefix"]="$count"
}

# Checks whether one distributed package matches the build checksum.
# @param project_dir Project directory.
# @param target_dir Compiled target directory.
# @return 0 when current, 1 when changed, 2 when checksum is missing.
dist_target_is_current() {
    local project_dir="$1"
    local target_dir="$2"
    local project_base name relative prefix checksum arch platform
    local package_name package_path sha file path count expected_count

    project_dir="${project_dir%/}"
    project_base=$(basename "$project_dir")
    name=$(project_name "$project_dir")
    relative="${target_dir#"$project_dir/bin/"}"
    prefix="$project_base/$relative"
    checksum="$target_dir/SHA256SUMS"
    IFS="/" read -r arch platform <<< "$relative"
    package_name="$name-$platform-$arch.zip"
    package_path="$project_base/$package_name"

    if [ ! -f "$checksum" ]; then
        echo "error: build checksum not found: $checksum" >&2
        echo "run ./scripts/build.sh $name" >&2
        return 2
    fi

    [ -f "$DIST_DIR/$package_path" ] || return 1
    [ -n "${MANIFEST_PACKAGE_SHA["$package_path"]:-}" ] || return 1

    count=0
    while read -r sha file; do
        [ -n "$sha" ] || continue
        [ -n "$file" ] || continue
        path="$prefix/$file"
        [ "${MANIFEST_SOURCE_SHA["$path"]:-}" = "$sha" ] || return 1
        count=$((count + 1))
    done < "$checksum"

    expected_count=${MANIFEST_SOURCE_COUNT["$prefix"]:-0}
    [ "$count" -eq "$expected_count" ] || return 1
    [ "$count" -gt 0 ] || return 1

    return 0
}

# Creates one changed target package and records its checksum.
# @param project_dir Project directory.
# @param target_dir Compiled target directory.
# @return 0 on success.
sync_target() {
    local project_dir="$1"
    local target_dir="$2"
    local project_base name relative arch platform executable package_name
    local project_dist package_path package work sha file size

    project_dir="${project_dir%/}"
    project_base=$(basename "$project_dir")
    name=$(project_name "$project_dir")
    relative="${target_dir#"$project_dir/bin/"}"
    IFS="/" read -r arch platform <<< "$relative"

    case "$platform" in
        windows) executable="$name.exe" ;;
        *) executable="$name" ;;
    esac

    if [ ! -f "$target_dir/$executable" ]; then
        echo "error: build artifact not found: $target_dir/$executable" >&2
        return 1
    fi

    project_dist="$DIST_DIR/$project_base"
    package_name="$name-$platform-$arch.zip"
    package="$project_dist/$package_name"
    package_path="$project_base/$package_name"
    mkdir -p "$project_dist"
    rm -f "$package"
    work=$(mktemp -d)

    while read -r sha file; do
        [ -n "$sha" ] || continue
        [ -n "$file" ] || continue
        if [ ! -f "$target_dir/$file" ]; then
            rm -rf "$work"
            echo "error: build artifact not found: $target_dir/$file" >&2
            return 1
        fi
        cp "$target_dir/$file" "$work/$file"
    done < "$target_dir/SHA256SUMS"

    [ -f "$project_dir/README.md" ] &&
        cp "$project_dir/README.md" "$work/README.md"
    [ -f "$project_dir/LICENSE" ] &&
        cp "$project_dir/LICENSE" "$work/LICENSE"
    [ -f "$project_dir/COPYING" ] &&
        cp "$project_dir/COPYING" "$work/COPYING"

    (
        cd "$work"
        zip -qr "$package" .
    )
    rm -rf "$work"

    MANIFEST_PACKAGE_SHA["$package_path"]=$(compute_sha256 "$package")
    size=$(stat -c%s "$package" 2>/dev/null || stat -f%z "$package")
    MANIFEST_PACKAGE_SIZE["$package_path"]="$size"
    record_target_source_state "$project_dir" "$target_dir"
}

# Removes distributed packages whose compiled targets no longer exist.
# @param project_dir Project directory.
# @return 0 when unchanged, 1 when stale packages were removed.
prune_stale_targets() {
    local project_dir="$1"
    local project_base name project_dist expected package package_name
    local target_dir relative arch platform package_path key rest prefix changed

    project_dir="${project_dir%/}"
    project_base=$(basename "$project_dir")
    name=$(project_name "$project_dir")
    project_dist="$DIST_DIR/$project_base"
    [ -d "$project_dist" ] || return 0

    expected=$(mktemp)
    for target_dir in "$project_dir"/bin/*/*; do
        [ -d "$target_dir" ] || continue
        relative="${target_dir#"$project_dir/bin/"}"
        IFS="/" read -r arch platform <<< "$relative"
        printf '%s\n' "$name-$platform-$arch.zip" >> "$expected"
    done
    LC_ALL=C sort -u -o "$expected" "$expected"

    changed=0
    for package in "$project_dist"/*.zip; do
        [ -f "$package" ] || continue
        package_name=$(basename "$package")
        if ! grep -Fqx "$package_name" "$expected"; then
            package_path="$project_base/$package_name"
            rm -f "$package"
            unset 'MANIFEST_PACKAGE_SHA[$package_path]'
            unset 'MANIFEST_PACKAGE_SIZE[$package_path]'
            changed=1
        fi
    done
    rm -f "$expected"

    for key in "${!MANIFEST_SOURCE_SHA[@]}"; do
        case "$key" in
            "$project_base"/*)
                rest="${key#"$project_base/"}"
                prefix="${rest%/*}"
                if [ ! -d "$project_dir/bin/$prefix" ]; then
                    unset 'MANIFEST_SOURCE_SHA[$key]'
                    unset 'MANIFEST_SOURCE_COUNT[$project_base/$prefix]'
                fi
                ;;
        esac
    done

    return "$changed"
}

# Synchronizes one project using build-generated target checksums.
# @param project_dir Project directory.
# @return 0 when changed, 1 when no work was needed.
package_project() {
    local project_dir="$1"
    local project_base target_dir status changed stale_changed

    project_dir="${project_dir%/}"
    project_base=$(basename "$project_dir")

    echo "Processing $project_base..."
    if [ ! -d "$project_dir/bin" ]; then
        echo "    [=] No build artifacts."
        return 1
    fi

    changed=0

    for target_dir in "$project_dir"/bin/*/*; do
        [ -d "$target_dir" ] || continue

        status=0
        dist_target_is_current "$project_dir" "$target_dir" || status=$?
        case "$status" in
            0)
                continue
                ;;
            1)
                sync_target "$project_dir" "$target_dir"
                changed=1
                ;;
            *)
                return "$status"
                ;;
        esac
    done

    stale_changed=0
    if ! prune_stale_targets "$project_dir"; then
        stale_changed=1
    fi

    if [ "$changed" -eq 1 ] || [ "$stale_changed" -eq 1 ]; then
        echo "    [+] Distribution updated."
        return 0
    fi

    echo "    [=] Distribution current."
    return 1
}

# Removes one deleted project's loaded manifest state.
# @param project_base Project directory basename.
# @return 0 on success.
drop_project_manifest_state() {
    local project_base="$1"
    local key

    for key in "${!MANIFEST_SOURCE_SHA[@]}"; do
        case "$key" in
            "$project_base"/*) unset 'MANIFEST_SOURCE_SHA[$key]' ;;
        esac
    done
    for key in "${!MANIFEST_SOURCE_COUNT[@]}"; do
        case "$key" in
            "$project_base"/*) unset 'MANIFEST_SOURCE_COUNT[$key]' ;;
        esac
    done
    for key in "${!MANIFEST_PACKAGE_SHA[@]}"; do
        case "$key" in
            "$project_base"/*)
                unset 'MANIFEST_PACKAGE_SHA[$key]'
                unset 'MANIFEST_PACKAGE_SIZE[$key]'
                ;;
        esac
    done
}

# Removes distributions for projects that no longer exist.
# @return 0 when unchanged, 1 when stale distributions were removed.
prune_stale_projects() {
    local project_dist project_base source_project changed

    changed=0
    for project_dist in "$DIST_DIR"/*.c/; do
        [ -d "$project_dist" ] || continue
        project_base=$(basename "$project_dist")
        source_project="$PROJ_DIR/$project_base"

        if ! is_project "$source_project"; then
            echo "Removing stale $project_base..."
            rm -rf "$project_dist"
            drop_project_manifest_state "$project_base"
            changed=1
        fi
    done

    return "$changed"
}

# Writes manifest.json from build and package checksums already in memory.
# @return 0 on success.
generate_manifest() {
    local manifest_file first_source first_project first_package updated_at
    local source_path project_dir project_base name project_dist target_dir
    local relative arch platform package_name package_path package sha size

    manifest_file="$DIST_DIR/manifest.json"
    first_source=true
    first_project=true
    updated_at=$(date -u +'%Y-%m-%dT%H:%M:%SZ')

    {
        echo "{"
        printf '  "updated_at": "%s",\n' "$updated_at"
        echo "  \"timestamp\": $(date -u +%s),"
        echo '  "sources": {'
    } > "$manifest_file"

    while IFS= read -r source_path; do
        [ -n "$source_path" ] || continue
        if [ "$first_source" = true ]; then
            first_source=false
        else
            echo "," >> "$manifest_file"
        fi
        printf '    "%s": "%s"' \
            "$source_path" "${MANIFEST_SOURCE_SHA["$source_path"]}" \
            >> "$manifest_file"
    done < <(
        printf '%s\n' "${!MANIFEST_SOURCE_SHA[@]}" | LC_ALL=C sort
    )

    if [ "$first_source" = false ]; then
        echo >> "$manifest_file"
    fi
    echo '  },' >> "$manifest_file"
    echo '  "projects": {' >> "$manifest_file"

    for project_dir in "$PROJ_DIR"/*.c/; do
        is_project "$project_dir" || continue
        project_dir="${project_dir%/}"
        project_base=$(basename "$project_dir")
        name=$(project_name "$project_dir")
        project_dist="$DIST_DIR/$project_base"
        [ -d "$project_dist" ] || continue

        first_package=true
        for target_dir in "$project_dir"/bin/*/*; do
            [ -d "$target_dir" ] || continue
            relative="${target_dir#"$project_dir/bin/"}"
            IFS="/" read -r arch platform <<< "$relative"
            package_name="$name-$platform-$arch.zip"
            package_path="$project_base/$package_name"
            package="$DIST_DIR/$package_path"
            [ -f "$package" ] || continue
            [ -n "${MANIFEST_PACKAGE_SHA["$package_path"]:-}" ] || continue

            if [ "$first_package" = true ]; then
                if [ "$first_project" = true ]; then
                    first_project=false
                else
                    echo "," >> "$manifest_file"
                fi
                echo "    \"$project_base\": [" >> "$manifest_file"
                first_package=false
            else
                echo "," >> "$manifest_file"
            fi

            sha="${MANIFEST_PACKAGE_SHA["$package_path"]}"
            size="${MANIFEST_PACKAGE_SIZE["$package_path"]:-}"
            if [ -z "$size" ]; then
                size=$(stat -c%s "$package" 2>/dev/null || stat -f%z "$package")
            fi

            cat <<MANIFEST_ENTRY >> "$manifest_file"
      {
        "arch": "$arch",
        "platform": "$platform",
        "binary": "$package_name",
        "size_bytes": $size,
        "sha256": "$sha",
        "path": "$package_path"
      }
MANIFEST_ENTRY
        done

        if [ "$first_package" = false ]; then
            printf '\n    ]' >> "$manifest_file"
        fi
    done

    printf '\n  }\n}\n' >> "$manifest_file"
}

# Dispatches one project distribution or all project distributions.
# @param target Logical kccli name or all.
# @return 0 on success.
main() {
    local target project_dir status changed stale_changed

    [ "$#" -eq 1 ] || {
        usage
        exit 1
    }

    target="$1"
    [ -d "$PROJ_DIR" ] || {
        echo "error: projects directory not found: $PROJ_DIR" >&2
        exit 1
    }

    mkdir -p "$DIST_DIR"
    load_manifest_state
    remove_dist_checksum_files

    stale_changed=0
    if ! prune_stale_projects; then
        stale_changed=1
    fi

    if [ "$target" = "all" ]; then
        changed="$stale_changed"

        for project_dir in "$PROJ_DIR"/*.c/; do
            is_project "$project_dir" || continue

            status=0
            package_project "$project_dir" || status=$?
            case "$status" in
                0) changed=1 ;;
                1) ;;
                *) exit "$status" ;;
            esac
        done

        if [ "$changed" -eq 1 ] || [ ! -f "$DIST_DIR/manifest.json" ]; then
            echo "Generating manifest.json..."
            generate_manifest
            echo "Done. Distribution updated in $DIST_DIR/."
        else
            echo "Done. No distribution changes."
        fi
        return 0
    fi

    case "$target" in
        *[!A-Za-z0-9_-]*|'')
            echo "error: invalid project name: $target" >&2
            usage
            exit 1
            ;;
    esac

    project_dir=$(resolve_project "$PROJ_DIR" "$target") || {
        echo "error: project not found: $target" >&2
        exit 1
    }

    status=0
    package_project "$project_dir" || status=$?
    case "$status" in
        0) changed=1 ;;
        1) changed="$stale_changed" ;;
        *) exit "$status" ;;
    esac

    if [ "$changed" -eq 1 ] || [ ! -f "$DIST_DIR/manifest.json" ]; then
        echo "Generating manifest.json..."
        generate_manifest
        echo "Done. Distribution updated in" \
            "$DIST_DIR/$(basename "$project_dir")/."
    else
        echo "Done. No distribution changes."
    fi
}

main "$@"
