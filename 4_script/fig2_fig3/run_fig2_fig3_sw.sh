#!/bin/bash

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

# num of apps
num_rows=7
# num of configurations
num_cols=3

client_core=12


# host sw app 1500 bytes, 8 cores
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=8
execution_time=60
for ((i = 0; i < num_rows; i++)); do
	for ((j = 0; j < num_cols; j++)); do
		config_value=${matrix[$i, $j]}
		if [[ -n "$config_value" ]]; then
			bash $REPO_PATH/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $HOST_MAC $HOST_IP $HOST_SSH_IP $client_core $server_core $server_core_start $pkt_size $((i + 1)) $config_value 1 h_a${i}_b${config_value}_c8_r10.txt 10 0 0 0 0 0 0 $execution_time
			sleep 10
		fi
	done
done

# snic sw app 1500 bytes, 8 cores
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=8
execution_time=60
for ((i = 0; i < num_rows; i++)); do
	for ((j = 0; j < num_cols; j++)); do
		config_value=${matrix[$i, $j]}
		if [[ -n "$config_value" ]]; then
			bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $SNIC_MAC $SNIC_IP $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size $((i + 1)) $config_value 1 n_a${i}_b${config_value}_c8_r10.txt 10 0 0 0 0 0 0 $execution_time
			sleep 10
		fi
	done
done