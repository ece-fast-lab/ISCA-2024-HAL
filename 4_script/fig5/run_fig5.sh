#!/bin/bash

# SNIC forward rates corresponding 0 Gbps, 10 Gbps, ..., 80 Gbps (only used for Software Split)
split_rate_array=("2" "3" "4" "5" "6")

# num of cores used in SNIC for splitting (only used for Software Split)
split_core_array=("1" "2" "4")

# application configuration matrix
declare -A matrix
# bm25
matrix[0, 0]=2000
matrix[0, 1]=4000
# bayes
matrix[1, 0]=128
matrix[1, 1]=256
# knn
matrix[2, 0]=8
matrix[2, 1]=16
# nat
matrix[3, 0]=1000
matrix[3, 1]=10000
# kvs
matrix[4, 0]=1
matrix[4, 1]=2
matrix[4, 2]=3
# count
matrix[5, 0]=4
matrix[5, 1]=8
# ema
matrix[6, 0]=4
matrix[6, 1]=8


client_core=12
server_core_start=0
server_core=8

# nat 1K, soft split
i=3
j=0
config_value=${matrix[$i, $j]}
pkt_size=508
execution_time=60
rate=8
for core in "${split_core_array[@]}"; do
    for split_rate in "${split_rate_array[@]}"; do
        bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $SNIC_MAC $SNIC_IP $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size 4 10 1 test.txt 8 0 0 0 0 0 0 $execution_time
        sleep 5
        bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $SNIC_MAC $SNIC_IP $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size $((i+1)) $config_value 1 n_sp_a${i}_b${config_value}_c${core}_s${split_rate}_r${rate}.txt $rate 0 2 $split_rate $core 0 0 $execution_time
        sleep 5
    done
done
