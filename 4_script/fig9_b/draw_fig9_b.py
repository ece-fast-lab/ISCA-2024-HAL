import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
pal = sns.color_palette("Paired")
import argparse

# Set up argument parser
parser = argparse.ArgumentParser(description='Process the file path.')
parser.add_argument('file_path', type=str, help='Path to the CSV file')

# Parse the arguments
args = parser.parse_args()
file_path = args.file_path

# Create a figure
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(16, 2.8) )
plt.rcParams['font.size'] = '14'

traffic_rate = [10,20,30,40,50,60,70,80,90,100]
traffic_rate_P = [0,10,20,30,40,50,60,70,80,90,100]

# Read REM data host/snic only
file_path_only = '../fig4/' + file_path
data = pd.read_csv(file_path_only)
rem_filtered_data = data[data['Index'].str.contains('rem_b0')]
rem_filtered_data['sort_key'] = rem_filtered_data['Index'].apply(lambda x: int(x.split('_r')[2]))
rem_sorted_data = rem_filtered_data.sort_values(by='sort_key')
rem_sorted_data = rem_sorted_data.drop(columns=['sort_key'])

rem_host_T = rem_sorted_data['T_h*.txt'].tolist()
rem_snic_T = rem_sorted_data['T_n*.txt'].tolist()
rem_host_L = rem_sorted_data['L_h*.txt'].tolist()
rem_snic_L = rem_sorted_data['L_n*.txt'].tolist()
rem_host_P = rem_sorted_data['P_h*.txt'].tolist()
rem_host_P = [x - 117 for x in rem_host_P]
rem_host_P = [rem_host_P[0]] + rem_host_P
rem_snic_P = rem_sorted_data['P_n*.txt'].tolist()
rem_snic_P = [x - 117 for x in rem_snic_P]
rem_snic_P = [rem_snic_P[0]] + rem_snic_P

# Read REM data HAL
data = pd.read_csv(file_path)
rem_filtered_data = data[data['Index'].str.contains('s_n')]
rem_filtered_data['sort_key'] = rem_filtered_data['Index'].apply(lambda x: int(x.split('_r')[1].split('_')[0]))
rem_sorted_data = rem_filtered_data.sort_values(by='sort_key')
rem_sorted_data = rem_sorted_data.drop(columns=['sort_key'])

rem_hal_host_T = rem_sorted_data['T_h*.txt'].tolist()
rem_hal_snic_T = rem_sorted_data['T_n*.txt'].tolist()
rem_hal_T = [x + y for x, y in zip(rem_hal_host_T, rem_hal_snic_T)]

rem_filtered_data = data[~data['Index'].str.contains('s_n')]
rem_filtered_data['sort_key'] = rem_filtered_data['Index'].apply(lambda x: int(x.split('_r')[1].split('_')[0]))
rem_sorted_data = rem_filtered_data.sort_values(by='sort_key')
rem_sorted_data = rem_sorted_data.drop(columns=['sort_key'])

rem_hal_L = rem_sorted_data['L_n*.txt'].tolist()
rem_hal_P = rem_sorted_data['P_n*.txt'].tolist()
rem_hal_P = [x - 117 for x in rem_hal_P]
rem_hal_P = [rem_hal_P[0]] + rem_hal_P

# Throughput
ax1.plot(traffic_rate,rem_host_T,'s-',color = pal[1],markersize='8',label="Host Processing")
ax1.plot(traffic_rate,rem_snic_T,'o-',color = pal[3],markersize='8',label="SNIC Processing")
ax1.plot(traffic_rate,rem_hal_T,'v-',color = "#F0BB41",markersize='8',label="HAL Processing")

ax1.set_xlabel("Traffic Rate (Gb/s)")
ax1.set_ylabel("Throughput (Gb/s)")
ax1.set_ylim(0,120)
ax1.yaxis.grid()

# p99 Latency
ax2.plot(traffic_rate,rem_host_L,'s-',color = pal[1],markersize='8',label="Host Processing")
ax2.plot(traffic_rate,rem_snic_L,'o--',color = pal[3],markersize='8',label="SmartNIC Processing")
ax2.plot(traffic_rate,rem_hal_L,'v-',color = "#F0BB41",markersize='8',label="HAL Processing")

ax2.set_xlabel("Traffic Rate (Gb/s)")
ax2.set_ylabel("p99 Latency (us)")
ax2.set_ylim(-10,1000)
ax2.yaxis.grid()

# Power
ax3.plot(traffic_rate_P,rem_host_P,'s-',color = pal[1],markersize='8',label="Host Processing")
ax3.plot(traffic_rate_P,rem_snic_P,'o--',color = pal[3],markersize='8',label="SmartNIC Processing")
ax3.plot(traffic_rate_P,rem_hal_P,'v-',color = "#F0BB41",markersize='8',label="HAL Processing")

ax3.set_xlabel("Traffic Rate (Gb/s)")
ax3.set_ylabel("Power (W)")
ax3.yaxis.grid()

# Create a single legend above the subplots
lines, labels = ax1.get_legend_handles_labels()
fig.legend(lines, labels, loc='upper center', ncol=len(labels), bbox_to_anchor=(0.5, 1.1), frameon=False)

# Adjust the layout
fig.tight_layout()

# Show the plot
plt.savefig("fig9_b.png", bbox_inches='tight')