#!/bin/bash

echo "dst ssh ip: $1" 
echo "dst type: $2" # 0: server, 1: SNIC
echo "file name: $3"
echo "time: $4"
echo "app type: $5" # 0: Crypto, 1: Compress
echo "app args: $6" # Crypto: 0: RSA, 1: DH, 2: DSA; Compress: 0: C, 1: D


DEST_SSH_IP=$1
DST_TYPE_ID=$2
FILENAME=$3
TIME=$4
APP_TYPE=$5
APP_ARGS=$6

# Determine the destination is a host or a SNIC
if [[ $DST_TYPE_ID == "0" ]]; then
    DST_TYPE="host"
else
    DST_TYPE="snic"
fi

if [[ $APP_TYPE == "0" ]]; then
    # Crypto
    if [[ $APP_ARGS == "0" ]]; then
        APP_ARGS_NAME="rsa2048"
    elif [[ $APP_ARGS == "1" ]]; then
        APP_ARGS_NAME="ecdhx25519"
    else
        APP_ARGS_NAME="ecdsap384"
    fi
    COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/crypto && echo $PASSWORD | sudo -S bash run_crypto_${DST_TYPE}.sh $APP_ARGS_NAME $FILENAME $TIME"

    # Double time for RSA and DSA
    if [[ $APP_ARGS == "0" || $APP_ARGS == "2" ]]; then
        TIME=$((TIME * 2))
    fi
else
    # Compress
    if [[ $DST_TYPE == "host" ]]; then
        # ~ 50s
        COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/compress && echo $PASSWORD | sudo -S bash run_compress_${DST_TYPE}.sh $APP_ARGS $FILENAME 1000"
    else
        # ~ 50s
        COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/compress && echo $PASSWORD | sudo -S bash run_compress_${DST_TYPE}.sh $APP_ARGS $FILENAME 50000"
    fi
fi

SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $DEST_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}
echo "$cmd"

# power measurement
P_COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/tools/power_measure && mkdir -p temp_results && echo $PASSWORD | sudo -S bash measure_power.sh $((TIME - 25)) > temp_results/$FILENAME"
P_SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $P_COMMAND_SSH"
P_cmd=${P_SSH_CMD}
echo "$P_cmd"

$cmd &
sleep 10
$P_cmd &
sleep $((TIME - 10))

mkdir -p results
scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/2_sw_src/tools/power_measure/temp_results/$FILENAME results/P_$FILENAME

if [[ $APP_TYPE == "0" ]]; then
    if [[ $DEST_SSH_IP == $HOST_SSH_IP ]]; then
        scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/3_benchmark/hw/crypto/temp_results/$FILENAME results/T_$FILENAME
    else
        scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $SNIC_ACCOUNT@$SNIC_SSH_IP:${REPO_PATH}/3_benchmark/hw/crypto/temp_results/$FILENAME results/T_$FILENAME
    fi
else
    if [[ $DEST_SSH_IP == $HOST_SSH_IP ]]; then
        scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/3_benchmark/hw/compress/temp_results/$FILENAME results/T_$FILENAME
    else
        scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $SNIC_ACCOUNT@$SNIC_SSH_IP:${REPO_PATH}/3_benchmark/hw/compress/temp_results/$FILENAME results/T_$FILENAME
    fi
fi
