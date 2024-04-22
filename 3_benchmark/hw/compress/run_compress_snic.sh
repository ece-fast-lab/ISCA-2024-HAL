#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <type> <filename> <iteration>"
    exit 1
fi

TYPE=$1
FILENAME=$2
ITERATION=$3

mkdir -p temp_results

sudo /home/ubuntu/dpdk-22.07/build/app/dpdk-test-compress-perf -l 0-2 -a 03:00.0,class=compress,mr_ext_memseg_en=0 \
  -- --driver-name mlx5_pci \
  --input-file Silesia/mozilla \
  --seg-sz 59460 \
  --compress-level 9 \
  --num-iter $ITERATION \
  --extended-input-sz 1048576 \
  --max-num-sgl-segs 18 \
  --huffman-enc fixed \
  > temp_results/$FILENAME