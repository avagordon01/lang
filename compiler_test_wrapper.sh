#!/usr/bin/env bash

set -euo pipefail

input="$1"
output="$(basename $1)"

./compiler ${input}.kl ${output}.ir
llc -filetype=obj ${output}.ir -o ${output}.o
if test -f ${input}.cc; then
    c++ ${input}.cc ${output}.o -o ${output}
    ./${output}
fi
