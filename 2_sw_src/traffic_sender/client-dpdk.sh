#!/bin/bash

echo "src mac: $1"
echo "src ip: $2"

echo "dst mac: $3"
echo "dst ip: $4"
echo "dst ssh ip: $5"

echo "client cores: $6"
echo "server cores: $7"
echo "server core start: $8"

echo "pkt size: $9"
echo "app id: ${10}"
echo "app arg 1: ${11}"
echo "app arg 2: ${12}"

echo "file name: ${13}"
echo "send rate: ${14}"
echo "PORT: ${15}"
echo "split: ${16}" # 0: no split, 1: hard split, 2: soft split, 3: HAL split
echo "split cap index: ${17}"
echo "split core: ${18}"
echo "trace: ${19}"
echo "feedback rate: ${20}"
echo "time: ${21}"

SRC_MAC=$1
SRC_IP=$2
DEST_MAC=$3
DEST_IP=$4
DEST_SSH_IP=$5
NUM_CORE=$6
SERVER_CORE=$7
SERVER_CORE_START=$8
PKT_SIZE=$9
APP_ID=${10}
APP_ARG1=${11}
APP_ARG2=${12}
FILENAME=${13}
RATE_INDEX=${14}
PORT=${15}
SPLIT=${16}
SPLIT_CAP_INDEX=${17}
SPLIT_CORE=${18}
TRACE=${19}
FEEDBACK_RATE=${20}
TIME=${21}

# translate Gbps to packets/second
if [[ $PKT_SIZE == "1476" ]]; then
	rateArray=("0" "835561.4973" "1671122.995" "2506684.492" "3342245.989" "4177807.487" "5013368.984" "5848930.481" "6684491.979" "7520053.476" "8355614.973")
else 
	rateArray=("1" "2349624.06" "4699248.12" "7048872.18" "9398496.241" "11748120.3" "14097744.36" "16447368.42" "18796992.48" "21146616.54" "23496240.6")
fi

# power measurement
if [[ $APP_ID == "33" ]]; then
	P_COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/tools/power_measure && mkdir -p temp_results && echo $PASSWORD | sudo -S bash measure_power.sh $((TIME - 50)) > temp_results/$FILENAME"
else 
	P_COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/tools/power_measure && mkdir -p temp_results && echo $PASSWORD | sudo -S bash measure_power.sh $((TIME - 30)) > temp_results/$FILENAME"
fi

P_SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $P_COMMAND_SSH"
P_cmd=${P_SSH_CMD}


if [[ $APP_ID == "33" ]]; then
	if [[ $APP_ARG1 == "0" ]]; then
        APP_ARGS_NAME="teakettle_2500"
    else
        APP_ARGS_NAME="snort_literals"
    fi

	if [[ $DEST_SSH_IP == $HOST_SSH_IP ]]; then
		DST_TYPE="host"
	else
		DST_TYPE="snic"
		if [[ $SPLIT == "1" ]]; then
			DST_TYPE="snic_hal"
		fi
	fi
	# REM
	if [[ $SPLIT == "1" ]]; then
		COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/rem && echo $PASSWORD | sudo -S bash run_rem_${DST_TYPE}.sh $APP_ARGS_NAME $FILENAME $((TIME - 40))"
	else 
		if [[ $SERVER_CORE == "16" ]]; then
			COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/rem && echo $PASSWORD | sudo -S bash run_rem_${DST_TYPE}_split.sh $APP_ARGS_NAME $FILENAME $((TIME - 40))"
		else
			COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/rem && echo $PASSWORD | sudo -S bash run_rem_${DST_TYPE}.sh $APP_ARGS_NAME $FILENAME $((TIME - 40))"
		fi
	fi
else 
	# traffic reciever
	COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/traffic_receiver && echo $PASSWORD | sudo -S bash server-dpdk.sh $SERVER_CORE $SERVER_CORE_START $APP_ID $APP_ARG1 $APP_ARG2 $FILENAME $PORT $PKT_SIZE $((TIME - 20))"
fi
SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $DEST_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}

# host reciever for hard/soft split
if [[ $APP_ID == "33" ]]; then
	# REM
	H_COMMAND_SSH="cd ${REPO_PATH}/3_benchmark/hw/rem && echo $PASSWORD | sudo -S bash run_rem_host_hal.sh $APP_ARGS_NAME $FILENAME $((TIME - 40))"
else
	H_COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/traffic_receiver && echo $PASSWORD | sudo -S bash server-dpdk.sh 8 0 $APP_ID $APP_ARG1 $APP_ARG2 $FILENAME $PORT $PKT_SIZE $((TIME - 20))"
fi
H_SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $H_COMMAND_SSH"
H_cmd=${H_SSH_CMD}

# soft split
S_COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/software_load_balancer && echo $PASSWORD | sudo -S bash splitter-dpdk.sh $SERVER_CORE $SERVER_CORE_START $APP_ID $APP_ARG1 $APP_ARG2 $FILENAME $PORT $((TIME - 10)) ${rateArray[$SPLIT_CAP_INDEX]} $SPLIT_CORE"
S_SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $DEST_SSH_IP $S_COMMAND_SSH"
S_cmd=${S_SSH_CMD}

# hard split
F_COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/traffic_receiver && echo $PASSWORD | sudo -S bash server-dpdk-feedback.sh $SERVER_CORE $SERVER_CORE_START $APP_ID $APP_ARG1 2 $FILENAME $PORT $PKT_SIZE $FEEDBACK_RATE $((TIME - 20))"
F_SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $DEST_SSH_IP $F_COMMAND_SSH"
F_cmd=${F_SSH_CMD}

# HAL split
DH_COMMAND_SSH="cd ${REPO_PATH}/2_sw_src/traffic_receiver && echo $PASSWORD | sudo -S bash server-dpdk-dynamic.sh 8 0 $APP_ID $APP_ARG1 1 $FILENAME $PORT $PKT_SIZE $((TIME - 20))"
DH_SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $DH_COMMAND_SSH"
DH_cmd=${DH_SSH_CMD}

# Experiemnt: Start the traffic sender
mkdir -p temp_results
echo "sudo timeout --signal=SIGINT $((TIME))s ${REPO_PATH}/2_sw_src/traffic_sender/dpdk-tx  -l 0-$NUM_CORE -a "$CLIENT_NIC_PCIE1" -a "$CLIENT_NIC_PCIE2" -- -p $PORT --source-mac="$SRC_MAC" --source-ip="$SRC_IP" --dest-mac="$DEST_MAC" --dest-ip="$DEST_IP" --size=$PKT_SIZE --rate=${rateArray[$RATE_INDEX]} --app-diff-ip=1 --trace=$TRACE > temp_results/$FILENAME &"
sudo timeout --signal=SIGINT $((TIME))s ${REPO_PATH}/2_sw_src/traffic_sender/dpdk-tx -l 0-$NUM_CORE -a "$CLIENT_NIC_PCIE1" -a "$CLIENT_NIC_PCIE2" -- -p $PORT --source-mac="$SRC_MAC" --source-ip="$SRC_IP" --dest-mac="$DEST_MAC" --dest-ip="$DEST_IP" --size=$PKT_SIZE --rate=${rateArray[$RATE_INDEX]} --app-diff-ip=1 --trace=$TRACE > temp_results/$FILENAME &
echo "started experiment"
sleep 5

# no split
if [[ $SPLIT == "0" ]]; then
	$cmd &
	echo "$cmd"
fi

# hard split
if [[ $SPLIT == "1" ]]; then
	echo "hard split!"
	$cmd &
	$H_cmd &
	echo "$cmd"
	echo "$H_cmd"
fi

# soft split
if [[ $SPLIT == "2" ]]; then
	echo "soft split!"
	$S_cmd &
	echo "$S_cmd"
	sleep 5
	$H_cmd &
	echo "$H_cmd"
fi

# HAL split
if [[ $SPLIT == "3" ]]; then
	echo "hal split!"
	$cmd &
	echo "$cmd"
	$DH_cmd &
	echo "$DH_cmd"
fi


sleep 5
# REM need more time to start
if [[ $APP_ID == "33" ]]; then
	sleep 20
fi

$P_cmd &
echo "$P_cmd"

sleep $((TIME - 5))

# Collect Results
mkdir -p results
cp temp_results/$FILENAME results/L_$FILENAME

scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/2_sw_src/tools/power_measure/temp_results/$FILENAME results/P_$FILENAME

if [[ $APP_ID == "33" ]]; then
	SRC_PATH="3_benchmark/hw/rem/temp_results"
else 
	SRC_PATH="2_sw_src/traffic_receiver/temp_results"
fi

if [[ $SPLIT == "1" ]]; then # hard split
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/${SRC_PATH}/$FILENAME results/T_hs_$FILENAME
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $SNIC_ACCOUNT@$SNIC_SSH_IP:${REPO_PATH}/${SRC_PATH}/$FILENAME results/T_ns_$FILENAME
elif [[ $SPLIT == "2" ]]; then # soft split
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/2_sw_src/traffic_receiver/temp_results/$FILENAME results/T_hs_$FILENAME
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $SNIC_ACCOUNT@$SNIC_SSH_IP:${REPO_PATH}/2_sw_src/software_load_balancer/temp_results/$FILENAME results/T_ns_$FILENAME
elif [[ $SPLIT == "3" ]]; then # HAL split
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/${SRC_PATH}/$FILENAME results/T_h_s_$FILENAME
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $SNIC_ACCOUNT@$SNIC_SSH_IP:${REPO_PATH}/${SRC_PATH}/$FILENAME results/T_n_s_$FILENAME
elif [[ $DEST_SSH_IP == $HOST_SSH_IP ]]; then
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $HOST_ACCOUNT@$HOST_SSH_IP:${REPO_PATH}/${SRC_PATH}/$FILENAME results/T_$FILENAME
else
	scp -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa $SNIC_ACCOUNT@$SNIC_SSH_IP:${REPO_PATH}/${SRC_PATH}/$FILENAME results/T_$FILENAME
fi
