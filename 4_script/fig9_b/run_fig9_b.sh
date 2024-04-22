#!/bin/bash

# Sending traffic rates corresponding 10 Gbps, 20 Gbps, ..., 100 Gbps
rate_array=("1" "2" "3" "4" "5" "6" "7" "8" "9" "10")

client_core=12
server_core=8
server_core_start=0
pkt_size=1476

split=1
i=32
j=0
execution_time=90
feedback_rate=380000

for rate in "${rate_array[@]}"; do
    COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/tools/forward_rate_setup && echo $PASSWORD | timeout --signal=SIGINT 5 ./snic_feedback 100000 ${feedback_rate} 7"
    SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $SNIC_SSH_IP $COMMAND_SSH"
    feedback_cmd=${SSH_CMD}
    $feedback_cmd
    sleep 20

    config_value=${matrix[$i, $j]}
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC_HAL $CLIENT_IP_HAL $SNIC_MAC_HAL $SNIC_IP_HAL $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size $((i+1)) $j 2 n_a${i}_b${config_value}_c${server_core}_r${rate}_sp.txt $rate 1 $split 0 0 0 0 $execution_time 
    sleep 20
done