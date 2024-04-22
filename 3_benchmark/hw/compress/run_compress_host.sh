#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <type> <filename> <iteration>"
    exit 1
fi

TYPE=$1
FILENAME=$2
ITERATION=$3

mkdir -p temp_results

script -c "bash run_compress_host_worker.sh $TYPE $ITERATION" temp_results/$FILENAME