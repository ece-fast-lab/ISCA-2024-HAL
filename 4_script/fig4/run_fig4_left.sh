#!/bin/bash

# Sending traffic rates corresponding 10 Gbps, 20 Gbps, ..., 100 Gbps
rate_array=("1" "2" "3" "4" "5" "6" "7" "8" "9" "10")

client_core=12

# host app, REM
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=16
execution_time=90
i=0
for rate in "${rate_array[@]}"; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $HOST_MAC $HOST_IP $HOST_SSH_IP $client_core $server_core $server_core_start $pkt_size 33 $i 1 h_rem_b${i}_c8_r${rate}.txt $rate 0 0 0 0 0 0 $execution_time
    sleep 10
done

# snic app, REM
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=8
execution_time=90
i=0
for rate in "${rate_array[@]}"; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $SNIC_MAC $SNIC_IP $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size 33 $i 1 n_rem_b${i}_c8_r${rate}.txt $rate 0 0 0 0 0 0 $execution_time
    sleep 10
done