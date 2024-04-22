#!/bin/bash

if [ -d "$REPO_PATH/2_sw_src/software_load_balancer" ]; then
    cd "$REPO_PATH/2_sw_src/software_load_balancer" && make clean
fi

if [ -d "$REPO_PATH/2_sw_src/traffic_receiver" ]; then
    cd "$REPO_PATH/2_sw_src/traffic_receiver" && make clean
fi

if [ -d "$REPO_PATH/2_sw_src/traffic_sender" ]; then
    cd "$REPO_PATH/2_sw_src/traffic_sender" && make clean
fi

if [ -d "$REPO_PATH/3_benchmark/hw/rem/rxpbench-22.10.0-lat" ]; then
    cd "$REPO_PATH/3_benchmark/hw/rem/rxpbench-22.10.0-lat" && make clean
fi

if [ -d "$REPO_PATH/3_benchmark/hw/rem" ]; then
    cd "$REPO_PATH/3_benchmark/hw/rem" && rm hsbench-samples.zip && rm -r hsbench-samples/
fi

if [ -d "$REPO_PATH/3_benchmark/hw/compress" ]; then
    cd "$REPO_PATH/3_benchmark/hw/compress" && rm -r Silesia/
fi