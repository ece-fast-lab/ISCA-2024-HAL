#!/bin/bash

# Check if hsbench-samples.zip exists
if [ ! -f hsbench-samples.zip ]; then
    # File does not exist, download it
    wget -O hsbench-samples.zip https://cdrdv2.intel.com/v1/dl/getContent/739375
    unzip hsbench-samples.zip
    mv "[Hyperscan] hsbench-samples/" hsbench-samples/
else
    # File already exists
    echo "hsbench-samples.zip already exists."
fi