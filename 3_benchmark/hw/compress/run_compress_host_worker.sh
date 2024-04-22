#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <type> <iteration>"
    exit 1
fi

TYPE=$1
iteration=$2

process=3
thread=1

cpu_list=40

for((numProc_comp = 0; numProc_comp < $process; numProc_comp ++))
do
    if [[ $TYPE == "0" ]]; then
        sudo taskset -c $cpu_list /QATzip/test/qatzip-test -i Silesia/mozilla -m 4 -l $iteration -t $thread -D comp &
    else 
        sudo taskset -c $cpu_list /QATzip/test/qatzip-test -i Silesia/mozilla -m 4 -l $iteration -t $thread -D decomp &
    fi
    cpu_list=$(($cpu_list + 1))
done
wait