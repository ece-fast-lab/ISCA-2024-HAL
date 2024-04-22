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

# Run the rxpbench command with the provided ruleset
sudo ./rxpbench-22.10.0-lat/build/rxpbench -D "-l 0-15 -n 6 --file-prefix x1 -a 25:00.0,class=eth" \
    --input-mode dpdk_port -1 25:00.0 \
    -d hs \
    -R hsbench-samples/pcre/$RULESET \
    -c 16 \
    -s $TIME \
    > temp_results/$FILENAME