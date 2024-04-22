#!/bin/bash

echo "result folder name: $1"

FOLDER_NAME=$1

cd ../fig2_fig3
python3 ../common/process_result.py results${FOLDER_NAME}/ results${FOLDER_NAME}/output.csv

cd ../fig4
python3 ../common/process_result.py results${FOLDER_NAME}/ results${FOLDER_NAME}/output.csv

cd ../fig5
python3 ../common/process_result_split.py results${FOLDER_NAME}/ results${FOLDER_NAME}/output.csv

cd ../fig9_a
python3 ../common/process_result.py results${FOLDER_NAME}/ results${FOLDER_NAME}/output.csv

cd ../fig9_b
python3 ../common/process_result.py results${FOLDER_NAME}/ results${FOLDER_NAME}/output.csv