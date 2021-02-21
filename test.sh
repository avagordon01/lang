#!/usr/bin/env bash
set -euxo pipefail
if [[ ! -d out ]]; then
    meson setup out
fi
meson compile -C out
meson test --print-errorlogs -C out
