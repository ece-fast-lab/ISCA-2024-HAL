#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <ruleset> <filename> <time>"
    exit 1
fi

# Assign the first argument to a variable
RULESET=$1
FILENAME=$2
TIME=$3

mkdir -p temp_results

sudo ./rxpbench-22.10.0-lat/build/rxpbench \
    -D "-l 0-2 -n 1 -a 03:00.0,class=regex -a auxiliary:mlx5_core.sf.3" \
    --input-mode dpdk_port -1 mlx5_core.sf.3 \
    -d rxp \
    -R /home/artifact/jinghan4/hsbench-samples/pcre-rxp/$RULESET \
    --latency-mode \
    -c 3 \
    -s $TIME \
    > temp_results/$FILENAME