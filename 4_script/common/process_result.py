import os
import glob
import csv
import re
import argparse

# Function to extract values from a file
def extract_value(file_path, file_type):
    with open(file_path, 'r') as file:
        lines = file.readlines()
    value = -1
    if file_type in ['P_h', 'P_n']:
        match = re.findall(r'\d+\.\d+', lines[-1])
        value = float(match[-1]) if match else None
    elif file_type in ['L_h', 'L_n']:
        for line in lines:
            if "99% tail latency" in line:
                match = re.search(r'\d+\.\d+', line)
                value = float(match.group()) if match else None
    elif file_type in ['T_h', 'T_n']:
        if 'rem' in file_path or 'a32' in file_path:
            for line in lines:
                if "PACKET PROCESSING PERF" in line:
                    match = re.search(r'\d+\.\d+', line)
                    value = float(match.group()) if match else None
                    break
        elif 'crypto' in file_path:
            match = re.findall(r'\d+\.\d+', lines[-1])
            value = float(match[-1]) if match else None
        elif 'compress' in file_path:
            if 'T_h' in file_path:
                value = 0
                for line in lines:
                    # Find all occurrences of the throughput in Gbps
                    matches = re.findall(r'(\d+\.\d+) Gbps', line)
                    for match in matches:
                        # Convert the throughput to float and add to the total
                        value += float(match)
            else:
                value = 0
                throughput_found = False
                for line in lines:
                    # Once we find the header, set a flag to start capturing throughput values in subsequent lines
                    if 'lcore id' in line and 'Comp [Gbps]' in line:
                        throughput_found = True
                    elif throughput_found:
                        parts = line.split()
                        if len(parts) > 4:
                            try:
                                # Assuming the throughput value is after "Comp ratio [%]"
                                if 'b0' in file_path:
                                    throughput = float(parts[4])
                                else:
                                    throughput = float(parts[5])
                                value += throughput
                            except ValueError:
                                continue  # Skip lines that don't match the expected format
        else:
            for line in lines:
                if "Total RX packet per second" in line:
                    match = re.search(r'\d+\.\d+', line)
                    value = float(match.group()) if match else None
                    break
    return value

# Set up argument parser
parser = argparse.ArgumentParser(description='Extract data from files and write to a CSV')
parser.add_argument('input_directory', type=str, help='The directory containing the text files')
parser.add_argument('output_csv', type=str, help='The output CSV file name')
args = parser.parse_args()

# Set the directory and CSV file from the parsed arguments
directory = args.input_directory
csv_file = args.output_csv

# Define the file patterns
patterns = ['P_h*.txt', 'T_h*.txt', 'L_h*.txt', 'P_n*.txt', 'T_n*.txt', 'L_n*.txt']

# Create a dictionary to store the extracted values
data = {}

# Iterate through the file patterns and the corresponding files
for pattern in patterns:
    file_type = pattern.split('*')[0]
    for file_path in sorted(glob.glob(os.path.join(directory, pattern))):
        index = os.path.basename(file_path).split(file_type)[1].split('.')[0]
        value = extract_value(file_path, file_type)
        if index not in data:
            data[index] = [0] * len(patterns)
        data[index][patterns.index(pattern)] = value

# Write the dictionary data to the CSV file
with open(csv_file, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(['Index'] + patterns)
    for key, values in data.items():
        csv_writer.writerow([key] + values)