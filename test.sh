#!/usr/bin/env bash
set -euxo pipefail
if [[ ! -d out ]]; then
    meson setup out
fi
meson compile -C out

for f in tests/*.kl; do
    echo
    echo $f
    out/alt-lexer < $f
    #gdb -ex "run < $f" --args out/alt-lexer
done
meson test --print-errorlogs -C out
