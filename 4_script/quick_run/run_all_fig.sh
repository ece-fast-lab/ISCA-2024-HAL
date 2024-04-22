#!/bin/bash

echo "result folder name: $1"

FOLDER_NAME=$1

source ../common/env_setup.sh

bash ../common/check_connection.sh
cd ../fig2_fig3
source ../common/env_setup.sh
bash run_fig2_fig3_sw.sh
sleep 60
source ../common/env_setup.sh
bash run_fig2_fig3_hw.sh
sleep 60
mv results/ results${FOLDER_NAME}/

bash ../common/check_connection.sh
cd ../fig4
source ../common/env_setup.sh
bash run_fig4_left.sh
sleep 60
source ../common/env_setup.sh
bash run_fig4_right.sh
sleep 60
mv results/ results${FOLDER_NAME}/

bash ../common/check_connection.sh
cd ../fig5
source ../common/env_setup.sh
bash run_fig5.sh
sleep 60
mv results/ results${FOLDER_NAME}/

bash ../common/check_connection_hal.sh
cd ../fig9_a
source ../common/env_setup.sh
bash run_fig9_a.sh
sleep 60
mv results/ results${FOLDER_NAME}/

bash ../common/check_connection_hal.sh
cd ../fig9_b
source ../common/env_setup.sh
bash run_fig9_b.sh
mv results/ results${FOLDER_NAME}/