#!/bin/bash

# Check if the time interval argument is provided
if [ $# -ne 1 ]; then
  echo "Usage: $0 <time_interval>"
  exit 1
fi

interval=$1
startTime=$(date +%s)
endTime=$((startTime + interval))
totalPower=0
count=0

mkdir -p temp_results

while [ $(date +%s) -lt $endTime ]; do
  # Execute the command and store the output in a variable
  output=$(sudo ipmitool dcmi power reading 2>/dev/null)

  # Check if the command succeeded
  if [ $? -eq 0 ]; then
    # Use grep and awk to extract the power value
    power=$(echo "$output" | grep -E "Instantaneous power reading:" | awk '{print $4}')

    # Output the power value
    echo "$power"

    # Update the total power and count
    totalPower=$(echo "$totalPower + $power" | bc)
    count=$((count + 1))
  else
    echo "Failed to read power value."
  fi

  # Sleep for 1 second
  sleep 1
done

# Calculate the average power
if [ $count -gt 0 ]; then
  averagePower=$(echo "scale=2; $totalPower / $count" | bc)
  echo "Average power reading over $count seconds: $averagePower Watts"
else
  echo "Failed to read power values."
fi
