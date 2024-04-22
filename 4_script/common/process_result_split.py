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
        # Extract the last integer from the last line
        match = re.findall(r'\d+\.\d+', lines[-1])
        value = float(match[-1]) if match else None
    elif file_type in ['L_h', 'L_n']:
        # Extract float from the line containing "99% tail latency"
        for line in lines:
            if "99% tail latency" in line:
                match = re.search(r'\d+\.\d+', line)
                value = float(match.group()) if match else None
    elif file_type == 'T_h':
        # Extract float from the line containing "Total RX packet per second"
        for line in lines:
            if "Total RX packet per second" in line:
                match = re.search(r'\d+\.\d+', line)
                value = float(match.group()) if match else None
                break
    elif file_type == 'T_n':
        # Extract float from the line containing "Total RX packet per second", "Total TX packet per second" and "Total work packet per second"
        value = [None, None, None]
        for line in lines:
            if "Total RX packet per second" in line:
                match = re.search(r'\d+\.\d+', line)
                value[0] = float(match.group()) if match else None
            elif "Total TX packet per second" in line:
                match = re.search(r'\d+\.\d+', line)
                value[1] = float(match.group()) if match else None
            elif "Total work packet per second" in line:
                match = re.search(r'\d+\.\d+', line)
                value[2] = float(match.group()) if match else None
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

# Define the column names
columns = ['P_h', 'T_h', 'L_h', 'P_n', 'T_n_rx', 'T_n_tx', 'T_n_work', 'L_n']

# Create a dictionary to store the extracted values
data = {}

# Iterate through the file patterns and the corresponding files
for pattern in patterns:
    file_type = pattern.split('*')[0]
    for file_path in sorted(glob.glob(os.path.join(directory, pattern))):
        # Extract the index number from the file name, if name starts with 's_n' then remove this prefix
        index = re.search(r'(s_n|_sp)_.*', os.path.basename(file_path)).group()
        if index.startswith('s_n'):
            index = index[3:]  # remove 's_n' prefix

        # Extract the value from the file
        value = extract_value(file_path, file_type)

        # Store the value in the dictionary
        if index not in data:
            data[index] = [0] * len(columns)
        if isinstance(value, list):
            for i, v in enumerate(value):
                data[index][columns.index(f'{file_type}_rx')+i] = v
        else:
            data[index][columns.index(file_type)] = value

# Write the dictionary data to the CSV file
with open(csv_file, 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(['Index'] + columns)  # Write the header row
    for key, values in data.items():
        csv_writer.writerow([key] + values)  # Write the data rows