#!/bin/bash

echo "core: $1"
echo "core start: $2"
echo "app id: $3"
echo "app arg 1: $4"
echo "app arg 2: $5"
echo "file name: $6"
echo "port: $7"
echo "pkt size: $8"
echo "feedback rate: $9"
echo "time: $10"

NUM_CORE=$1
CORE_START=$2
APP_ID=$3
APP_ARG1=$4
APP_ARG2=$5
FILENAME=$6
PORT=$7
PKT_SIZE=$8
FEEDBACK_RATE=$9
TIME=$10

mkdir -p temp_results
timeout --signal=SIGINT ${TIME}s ./dpdk-rx -l $CORE_START-$((NUM_CORE+CORE_START-1)) -- -p $PORT -a $APP_ID -b $APP_ARG1 -c $APP_ARG2 -s $PKT_SIZE -f $FEEDBACK_RATE > temp_results/$FILENAME
tail -n $((NUM_CORE+2)) $FILENAME