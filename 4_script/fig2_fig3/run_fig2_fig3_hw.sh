#!/bin/bash

client_core=12

## Host
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=8

# Host Crypto Setup
COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/crypto && echo $PASSWORD | sudo -S bash env_setup_crypto_host.sh"
SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}
echo "$cmd"
$cmd
sleep 20

# Crypto
execution_time=60
for i in {0..2}; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-hw.sh $HOST_SSH_IP 0 h_crypto_b${i}.txt $execution_time 0 $i
    sleep 20
done

# Host Compress Setup
COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/compress && echo $PASSWORD | sudo -S bash env_setup_compress_host.sh"
SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}
echo "$cmd"
$cmd
sleep 20

# Compress
execution_time=50
for i in {0..1}; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-hw.sh $HOST_SSH_IP 0 h_compress_b${i}.txt $execution_time 1 $i
    sleep 20
done

# REM
execution_time=90
for i in {0..1}; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $HOST_MAC $HOST_IP $HOST_SSH_IP $client_core $server_core $server_core_start $pkt_size 33 $i 1 h_rem_b${i}_c8_r10.txt 8 0 0 0 0 0 0 $execution_time
    sleep 20
done


# SNIC
pkt_size=1476 # 1500 = 1476 + 24
server_core_start=0
server_core=8

# Crypto
execution_time=60
for i in {0..2}; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-hw.sh $SNIC_SSH_IP 1 n_crypto_b${i}.txt $execution_time 0 $i
    sleep 20
done

# Compress
execution_time=50
bash ${REPO_PATH}/2_sw_src/traffic_sender/client-hw.sh $SNIC_SSH_IP 1 n_compress_b0.txt $execution_time  1 0
sleep 20
cp results/T_n_compress_b0.txt results/T_n_compress_b1.txt
cp results/P_n_compress_b0.txt results/P_n_compress_b1.txt

# REM
execution_time=90
for i in {0..1}; do
    bash ${REPO_PATH}/2_sw_src/traffic_sender/client-dpdk.sh $CLIENT_MAC $CLIENT_IP $SNIC_MAC $SNIC_IP $SNIC_SSH_IP $client_core $server_core $server_core_start $pkt_size 33 $i 1 n_rem_b${i}_c8_r10.txt 10 0 0 0 0 0 0 $execution_time
    sleep 20
done