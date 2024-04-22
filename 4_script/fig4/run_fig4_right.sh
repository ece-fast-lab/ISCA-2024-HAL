#!/bin/bash

# Sending traffic rates corresponding 10 Gbps, 20 Gbps, ..., 100 Gbps
rate_array=("1" "2" "3" "4" "5" "6" "7" "8" "9" "10")

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

# host app, nat 10K
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=8
execution_time=60
i=3
j=1
config_value=${matrix[$i, $j]}
for rate in "${rate_array[@]}"; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $HOST_MAC $HOST_IP $HOST_SSH_IP $client_core $server_core $server_core_start $pkt_size $((i + 1)) $config_value 1 h_a${i}_b${config_value}_c8_r${rate}.txt $rate 0 0 0 0 0 0 $execution_time
    sleep 10
done


# snic app, nat 10K
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=8
execution_time=60
i=3
j=1
config_value=${matrix[$i, $j]}
for rate in "${rate_array[@]}"; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $SNIC_MAC $SNIC_IP $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size $((i + 1)) $config_value 1 n_a${i}_b${config_value}_c8_r${rate}.txt $rate 0 0 0 0 0 0 $execution_time
    sleep 10
done