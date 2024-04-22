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
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(16, 2.8))
plt.rcParams['font.size'] = '14'

traffic_rate = [10,20,30,40,50,60,70,80,90,100]
traffic_rate_P = [0,10,20,30,40,50,60,70,80,90,100]

# Read NAT data host/snic only
file_path_only = '../fig4/' + file_path
data = pd.read_csv(file_path_only)
nat_filtered_data = data[data['Index'].str.contains('a3_b10000')]
nat_filtered_data['sort_key'] = nat_filtered_data['Index'].apply(lambda x: int(x.split('_r')[1]))
nat_sorted_data = nat_filtered_data.sort_values(by='sort_key')
nat_sorted_data = nat_sorted_data.drop(columns=['sort_key'])

nat_host_T = nat_sorted_data['T_h*.txt'].tolist()
nat_host_T = [x*1500*8/1000 for x in nat_host_T]
nat_snic_T = nat_sorted_data['T_n*.txt'].tolist()
nat_snic_T = [x*1500*8/1000 for x in nat_snic_T]
nat_host_L = nat_sorted_data['L_h*.txt'].tolist()
nat_snic_L = nat_sorted_data['L_n*.txt'].tolist()
nat_host_P = nat_sorted_data['P_h*.txt'].tolist()
nat_host_P = [x - 117 for x in nat_host_P]
nat_host_P = [nat_host_P[0]] + nat_host_P
nat_snic_P = nat_sorted_data['P_n*.txt'].tolist()
nat_snic_P = [x - 117 for x in nat_snic_P]
nat_snic_P = [nat_snic_P[0]] + nat_snic_P

# Read NAT data HAL
data = pd.read_csv(file_path)
nat_filtered_data = data[data['Index'].str.contains('s_n')]
nat_filtered_data['sort_key'] = nat_filtered_data['Index'].apply(lambda x: int(x.split('_r')[1].split('_')[0]))
nat_sorted_data = nat_filtered_data.sort_values(by='sort_key')
nat_sorted_data = nat_sorted_data.drop(columns=['sort_key'])

nat_hal_host_T = nat_sorted_data['T_h*.txt'].tolist()
nat_hal_host_T = [x*1500*8/1000 for x in nat_hal_host_T]
nat_hal_snic_T = nat_sorted_data['T_n*.txt'].tolist()
nat_hal_snic_T = [x*1500*8/1000 for x in nat_hal_snic_T]
nat_hal_T = [x + y for x, y in zip(nat_hal_host_T, nat_hal_snic_T)]

nat_filtered_data = data[~data['Index'].str.contains('s_n')]
nat_filtered_data['sort_key'] = nat_filtered_data['Index'].apply(lambda x: int(x.split('_r')[1].split('_')[0]))
nat_sorted_data = nat_filtered_data.sort_values(by='sort_key')
nat_sorted_data = nat_sorted_data.drop(columns=['sort_key'])

nat_hal_L = nat_sorted_data['L_n*.txt'].tolist()
nat_hal_P = nat_sorted_data['P_n*.txt'].tolist()
nat_hal_P = [x - 117 for x in nat_hal_P]
nat_hal_P = [nat_hal_P[0]] + nat_hal_P

# Throughput
ax1.plot(traffic_rate,nat_host_T,'s-',color = pal[1],markersize='8',label="Host Processing")
ax1.plot(traffic_rate,nat_snic_T,'o-',color = pal[3],markersize='8',label="SNIC Processing")
ax1.plot(traffic_rate,nat_hal_T,'v-',color = "#F0BB41",markersize='8',label="HAL Processing")

ax1.set_xlabel("Traffic Rate (Gb/s)")
ax1.set_ylabel("Throughput (Gb/s)")
ax1.set_ylim(0,120)
ax1.yaxis.grid()

# p99 Latency
ax2.plot(traffic_rate,nat_host_L,'s-',color = pal[1],markersize='8',label="Host Processing")
ax2.plot(traffic_rate,nat_snic_L,'o--',color = pal[3],markersize='8',label="SmartNIC Processing")
ax2.plot(traffic_rate,nat_hal_L,'v-',color = "#F0BB41",markersize='8',label="HAL Processing")

ax2.set_xlabel("Traffic Rate (Gb/s)")
ax2.set_ylabel("p99 Latency (us)")
ax2.set_ylim(-20,2000)
ax2.yaxis.grid()


ax3.plot(traffic_rate_P,nat_host_P,'s-',color = pal[1],markersize='8',label="Host Processing")
ax3.plot(traffic_rate_P,nat_snic_P,'o--',color = pal[3],markersize='8',label="SmartNIC Processing")
ax3.plot(traffic_rate_P,nat_hal_P,'v-',color = "#F0BB41",markersize='8',label="HAL Processing")

ax3.set_xlabel("Traffic Rate (Gb/s)")
ax3.set_ylabel("Power (W)")
ax3.yaxis.grid()

# Create a single legend above the subplots
lines, labels = ax1.get_legend_handles_labels()
fig.legend(lines, labels, loc='upper center', ncol=len(labels), bbox_to_anchor=(0.5, 1.1), frameon=False)


# Adjust the layout
fig.tight_layout()

# Show the plot
plt.savefig("fig9_a.png", bbox_inches='tight')