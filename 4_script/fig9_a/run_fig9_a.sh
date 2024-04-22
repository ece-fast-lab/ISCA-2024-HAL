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
# emas
matrix[6, 0]=4
matrix[6, 1]=8

client_core=12
server_core=8
server_core_start=0
pkt_size=1476

split=1
i=3
j=1
execution_time=60
feedback_rate=504300

for rate in "${rate_array[@]}"; do
    COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/tools/forward_rate_setup && echo $PASSWORD | timeout --signal=SIGINT 5 ./snic_feedback 100000 ${feedback_rate} 7"
    SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $SNIC_SSH_IP $COMMAND_SSH"
    feedback_cmd=${SSH_CMD}
    $feedback_cmd
    sleep 20

    config_value=${matrix[$i, $j]}
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC_HAL $CLIENT_IP_HAL $SNIC_MAC_HAL $SNIC_IP_HAL $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size $((i+1)) $config_value 2 n_a${i}_b${config_value}_c${server_core}_r${rate}_sp.txt $rate 1 $split 0 0 0 0 $execution_time 
    sleep 20
done