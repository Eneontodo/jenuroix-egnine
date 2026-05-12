#!/usr/bin/env bash
# Build jenuroix_compiler one time from the C++ source
# After that, you can run ./jenuroix_compiler directly.
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$ROOT/jenuroix_compiler.cpp"
OUT="$ROOT/jenuroix_compiler"

echo ""
echo "  Jenuroix Compiler — bootstrap"
echo "  Compiling jenuroix_compiler.cpp → jenuroix_compiler"
echo ""


CXX=""
for TRY in g++ clang++ c++; do
    if command -v "$TRY" &>/dev/null; then
        CXX="$TRY"
        break
    fi
done

if [ -z "$CXX" ]; then
    echo "[ERR] No C++ compiler found."
    echo "  Ubuntu/Debian:  sudo apt install build-essential"
    echo "  Fedora:         sudo dnf install gcc-c++"
    echo "  Arch:           sudo pacman -S base-devel"
    echo "  macOS:          xcode-select --install"
    exit 1
fi

echo "[OK] Using: $CXX"
"$CXX" -std=c++17 -O2 -Wall "$SRC" -o "$OUT"

chmod +x "$OUT"

echo ""
echo "[OK] Done!  jenuroix_compiler is ready."
echo ""
echo "Usage:"
echo "  ./jenuroix_compiler main.cpp --run"
echo "  ./jenuroix_compiler --editor"
echo "  ./jenuroix_compiler --clean --debug"
echo ""
echo "  # Или добавь в PATH чтобы вызывать из любой папки:"
echo "  export PATH=\"\$PATH:$ROOT\""
echo ""
