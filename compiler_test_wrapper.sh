#!/usr/bin/env bash

set -eu

type="$1"

input_raw="$2"
input="${input_raw%.*}"
output_raw="$(basename ${input}).ir"
output="${output_raw%.*}"

./compiler ${input_raw} ${output_raw}
if [ $type != "parse" ]; then
    llc -filetype=obj ${output_raw} -o ${output}.o
    if [ $type == "exe" ]; then
        c++ ${input}.cc ${output}.o -o ${output}
        ./${output}
    fi
fi
