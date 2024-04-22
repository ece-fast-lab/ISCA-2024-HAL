#!/bin/bash

echo "core: $1"
echo "core start: $2"
echo "app id: $3"
echo "app arg 1: $4"
echo "app arg 2: $5"
echo "file name: $6"
echo "port: $7"
echo "time: $8"
echo "splitter cap: $9"
echo "split core: $10"

NUM_CORE=$1
CORE_START=$2
APP_ID=$3
APP_ARG1=$4
APP_ARG2=$5
FILENAME=$6
PORT=$7
TIME=$8
SPLIT_CAP=$9
SPLIT_CORE=${10}

mkdir -p temp_results
timeout --signal=SIGINT ${TIME}s ./dpdk-splitter -l $CORE_START-$((NUM_CORE+CORE_START-1)) -- --dest-mac="08:c0:eb:bf:ef:3a" -f 0 -p $PORT -q 3 -a $APP_ID -b $APP_ARG1 -c $SPLIT_CAP -d $APP_ARG2 -s $SPLIT_CORE > temp_results/$FILENAME
tail -n $((NUM_CORE+2)) $FILENAME