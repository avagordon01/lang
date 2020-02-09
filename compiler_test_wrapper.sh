#!/usr/bin/env bash

set -euo pipefail

type="$1"

input_raw="$2"
input="${input_raw%.*}"
output_raw="$(basename ${input}).ir"
output="${output_raw%.*}"
print_ast=false
print_ir=false

if [ "$print_ast" = true ]; then
    gdb -ex "break main.cc:15" \
        -ex run \
        -ex "print driver.program_ast" \
        -ex continue \
        -ex quit \
        --args \
        ./compiler ${input_raw} ${output_raw}
else
    ./compiler ${input_raw} ${output_raw}
fi

if [ "$print_ir" = true ]; then
    cat ${output_raw}
fi

if [ $type != "parse" ]; then
    llc -filetype=obj ${output_raw} -o ${output}.o
    if [ $type == "exe" ]; then
        c++ ${input}.cc ${output}.o -o ${output}
        ./${output}
    fi
fi
