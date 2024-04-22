#!/bin/bash

# Define the directory name
directory="Silesia"

# Check if the directory exists
if [ ! -d "$directory" ]; then
    echo "Directory $directory does not exist. Creating it now..."
    mkdir -p "$directory"
    cd "$directory"

    # Download the file
    wget https://sun.aei.polsl.pl//~sdeor/corpus/silesia.zip

    # Unzip the downloaded file
    unzip silesia.zip
else
    echo "Directory $directory already exists."
fi