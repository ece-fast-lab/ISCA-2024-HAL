#!/bin/bash

echo "result folder name: $1"

FOLDER_NAME=$1

cd ../fig2_fig3
python3 draw_fig2.py results${FOLDER_NAME}/output.csv
python3 draw_fig3.py results${FOLDER_NAME}/output.csv

cd ../fig4
python3 draw_fig4_a.py results${FOLDER_NAME}/output.csv
python3 draw_fig4_b.py results${FOLDER_NAME}/output.csv

cd ../fig5
python3 draw_fig5.py results${FOLDER_NAME}/output.csv

cd ../fig9_a
python3 draw_fig9_a.py results${FOLDER_NAME}/output.csv

cd ../fig9_b
python3 draw_fig9_b.py results${FOLDER_NAME}/output.csv