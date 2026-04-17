#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP_BIN="${TMPDIR:-/tmp}/phase2_tree_demo"

CFLAGS="-Wall -Wextra -O2 -I$ROOT_DIR"
LDFLAGS="-lcrypto"

if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists openssl 2>/dev/null; then
  CFLAGS="$CFLAGS $(pkg-config --cflags openssl)"
  LDFLAGS="$(pkg-config --libs openssl)"
elif command -v brew >/dev/null 2>&1; then
  for formula in openssl@3 openssl; do
    prefix=$(brew --prefix "$formula" 2>/dev/null || true)
    if [ -n "$prefix" ] && [ -d "$prefix/include" ] && [ -d "$prefix/lib" ]; then
      CFLAGS="$CFLAGS -I$prefix/include"
      LDFLAGS="-L$prefix/lib -lcrypto"
      break
    fi
  done
fi

cd "$ROOT_DIR"
cc $CFLAGS tools/phase2_tree_demo.c object.c tree.c -o "$TMP_BIN" $LDFLAGS

output=$("$TMP_BIN")
printf '%s\n' "$output"

tree_path=$(printf '%s\n' "$output" | sed -n 's/^Root tree path: //p')
if [ -z "$tree_path" ]; then
  echo "Failed to locate tree object path." >&2
  exit 1
fi

echo
echo "Tree objects:"
find .pes/objects -type f | sort
echo
echo "Raw tree object (use this for screenshot 2B):"
xxd "$tree_path" | head -20
