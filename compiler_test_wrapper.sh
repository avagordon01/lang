#!/usr/bin/env bash

type="$1"
shift

set -eux

input_raw="$1"
input="${input_raw%.*}"
output_raw="$(basename ${input}).ir"
output="${output_raw%.*}"

./compiler ${input_raw} ${output_raw}
llc -filetype=obj ${output_raw} -o ${output}.o
if $type == "exe"; then
    c++ ${input}.cc ${output}.o -o ${output}
    ./${output}
fi
