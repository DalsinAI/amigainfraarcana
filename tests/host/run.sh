#!/bin/sh
# Host tests of the Amiga port's changes (no SDL needed): src/paths.cpp built
# as upstream builds it and as the Amiga build does (-D__amigaos__), and
# amiga/lgamma.c against the host's lgamma().
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
CXX=${CXX:-g++}
INC="-I$HERE/stubs -I$ROOT/include -I$ROOT/third_party/mINI-0.9.17/src/mini"
for mode in upstream amiga; do
    defs="-DNDEBUG"
    [ "$mode" = amiga ] && defs="$defs -D__amigaos__"
    echo "== $mode"
    $CXX -std=c++17 -Wall -Wextra -Wno-unused-value $defs $INC -o "$WORK/test_$mode" \
        "$HERE/test_paths.cpp" "$ROOT/src/paths.cpp"
    mkdir -p "$WORK/run_$mode"
    "$WORK/test_$mode" "$WORK/run_$mode"
done
echo "== lgamma"
${CC:-cc} -std=c99 -Wall -O2 -o "$WORK/test_lgamma" "$HERE/test_lgamma.c" "$ROOT/amiga/lgamma.c" -lm
"$WORK/test_lgamma"
echo "ALL PASS"
