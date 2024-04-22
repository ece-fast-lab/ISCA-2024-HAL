#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <algorithm> <filename> <time>"
    exit 1
fi

ALGORITHM=$1
FILENAME=$2
TIME=$3

OPENSSL_BIN="/usr/local/ssl/bin/openssl"

mkdir -p temp_results

# Run the OpenSSL speed test for the specified algorithm and time
sudo $OPENSSL_BIN speed \
    -engine qatengine \
    -async_jobs 8 \
    -seconds $TIME \
    $ALGORITHM \
    > temp_results/$FILENAME