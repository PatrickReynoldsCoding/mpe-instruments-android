#!/bin/sh
# Build every controller here into a package: controllers/build/<name>.odoc
#   ./build.sh [name ...]
# Each controllers/<name>/ holds controller.json and its C sources. Needs clang and wasm-ld.
set -e
here=$(cd "$(dirname "$0")" && pwd)
out="$here/build"
mkdir -p "$out"
names="$*"
[ -n "$names" ] || names=$(cd "$here" && ls -d */ | tr -d / | grep -v -e '^build$' -e '^include$')
for n in $names; do
    dir="$here/$n"
    if [ ! -f "$dir/controller.json" ] || ! ls "$dir"/*.c >/dev/null 2>&1; then
        echo "skipping $n: no controller.json or no .c files yet" >&2
        continue
    fi
    tmp=$(mktemp -d)
    clang --target=wasm32 -O2 -nostdlib -ffreestanding -fno-builtin-memset -Wall -Wextra -Werror \
          -I "$here/include" -Wl,--no-entry -Wl,--allow-undefined -Wl,--strip-all \
          -Wl,-z,stack-size=65536 -Wl,--initial-memory=1048576 \
          -o "$tmp/controller.wasm" "$dir"/*.c
    cp "$dir/controller.json" "$tmp/"
    [ -f "$dir/icon.png" ] && cp "$dir/icon.png" "$tmp/"
    rm -f "$out/$n.odoc"
    (cd "$tmp" && python3 -c "import sys, zipfile; z = zipfile.ZipFile(sys.argv[1], 'w', zipfile.ZIP_DEFLATED); [z.write(f) for f in sys.argv[2:]]; z.close()" \
        "$out/$n.odoc" controller.json controller.wasm $(ls icon.png 2>/dev/null))
    echo "$out/$n.odoc ($(wc -c < "$tmp/controller.wasm") bytes of WebAssembly)"
    rm -rf "$tmp"
done
