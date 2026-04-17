#!/usr/bin/env bash
# demo.sh — Demonstrate all four diary programs.
# Usage: ./demo.sh [build_dir]
#   build_dir defaults to ./build

set -euo pipefail

BUILD="${1:-./build}"
DIARY="$BUILD/diary.dat"

export DIARY_FILE="$DIARY"
rm -f "$DIARY"

echo "===== 1. pdadd: Add three entries ====="
printf "Today I started the diary project.\n.\n" | "$BUILD/pdadd" 2026-04-15
printf "Debugging the binary serializer took all afternoon.\n.\n" | "$BUILD/pdadd" 2026-04-16
printf "All four programs are done!\n.\n" | "$BUILD/pdadd" 2026-04-17

echo ""
echo "===== 2. pdlist: List all entries ====="
"$BUILD/pdlist"

echo ""
echo "===== 3. pdlist with date range (2026-04-15 to 2026-04-16) ====="
"$BUILD/pdlist" 2026-04-15 2026-04-16

echo ""
echo "===== 4. pdshow: Show entry for 2026-04-17 ====="
"$BUILD/pdshow" 2026-04-17

echo ""
echo "===== 5. pdadd: Overwrite existing entry ====="
printf "Rewritten: finished early, going for a walk.\n.\n" | "$BUILD/pdadd" 2026-04-16
echo "--- After overwrite ---"
"$BUILD/pdshow" 2026-04-16

echo ""
echo "===== 6. pdremove: Remove entry for 2026-04-15 ====="
"$BUILD/pdremove" 2026-04-15
echo "--- After removal ---"
"$BUILD/pdlist"

echo ""
echo "===== 7. pdremove: Non-existent date (expect error) ====="
"$BUILD/pdremove" 2020-01-01 || echo "(exit code: $?)"

rm -f "$DIARY"
echo ""
echo "===== Demo complete ====="
