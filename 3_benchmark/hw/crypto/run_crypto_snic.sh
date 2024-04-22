#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <algorithm> <filename> <time>"
    exit 1
fi

ALGORITHM=$1
FILENAME=$2
TIME=$3

mkdir -p temp_results

# Run the OpenSSL speed test for the specified algorithm and time
openssl speed \
    -engine pka \
    -seconds $TIME \
    $ALGORITHM \
    > temp_results/$FILENAME