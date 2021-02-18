#!/usr/bin/env bash
set -euxo pipefail
./build.sh
meson test --print-errorlogs -C out
