#!/bin/bash

export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig/
if [ -d "$REPO_PATH/2_sw_src/traffic_sender" ]; then
    cd "$REPO_PATH/2_sw_src/traffic_sender" && make -j
fi

if [ -d "$REPO_PATH/3_benchmark/hw/rem" ]; then
    cd "$REPO_PATH/3_benchmark/hw/rem" && bash download_rem_ruleset.sh
fi

if [ -d "$REPO_PATH/3_benchmark/hw/compress" ]; then
    cd "$REPO_PATH/3_benchmark/hw/compress" && bash download_compress_data.sh
fi