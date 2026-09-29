#!/bin/sh
# kccli distribution tool
# Summary: Packages statically linked CLI binaries by platform and architecture.
# Author:  KaisarCode
# Website: https://kaisarcode.com
# License: GNU General Public License v3.0

set -eu

script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(dirname "$script_dir")
bin_dir="$root_dir/bin"
dist_dir="$root_dir/dist/init"

compute_sha256()
{
    file=$1

    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$file" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$file" | awk '{print $1}'
    else
        openssl dgst -sha256 "$file" | awk '{print $NF}'
    fi
}

rm -rf "$root_dir/dist"
mkdir -p "$dist_dir"

for arch_dir in "$bin_dir"/*; do
    [ -d "$arch_dir" ] || continue
    arch=$(basename "$arch_dir")

    for platform_dir in "$arch_dir"/*; do
        [ -d "$platform_dir" ] || continue
        platform=$(basename "$platform_dir")

        case "$platform" in
            windows) executable="init.exe" ;;
            *) executable="init" ;;
        esac

        [ -f "$platform_dir/$executable" ] || continue

        package_name="init-$platform-$arch.zip"
        package="$dist_dir/$package_name"
        work=$(mktemp -d)

        cp "$platform_dir/$executable" "$work/$executable"
        cp "$root_dir/README.md" "$work/README.md"
        cp "$root_dir/LICENSE" "$work/LICENSE"

        (
            cd "$work"
            zip -qr "$package" .
        )
        rm -rf "$work"
        printf '%s  %s\n' "$(compute_sha256 "$package")" "$package_name"
    done
done > "$dist_dir/SHA256SUMS"

echo "Done. Packages ready in $dist_dir/"
