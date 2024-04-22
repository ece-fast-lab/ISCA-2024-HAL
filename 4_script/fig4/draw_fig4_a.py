import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
pal = sns.color_palette("Paired")
import pandas as pd
import argparse

# Set up argument parser
parser = argparse.ArgumentParser(description='Process the file path.')
parser.add_argument('file_path', type=str, help='Path to the CSV file')

# Parse the arguments
args = parser.parse_args()
file_path = args.file_path

data = pd.read_csv(file_path)

traffic_rate = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]

# Read NAT data
nat_filtered_data = data[data['Index'].str.contains('a3_b1000')]
nat_filtered_data['sort_key'] = nat_filtered_data['Index'].apply(lambda x: int(x.split('_r')[1]))
nat_sorted_data = nat_filtered_data.sort_values(by='sort_key')
nat_sorted_data = nat_sorted_data.drop(columns=['sort_key'])

nat_host_T = nat_sorted_data['T_h*.txt'].tolist()
nat_host_T = [x*1500*8/1000 for x in nat_host_T]
nat_host_L = nat_sorted_data['L_h*.txt'].tolist()
nat_snic_T = nat_sorted_data['T_n*.txt'].tolist()
nat_snic_T = [x*1500*8/1000 for x in nat_snic_T]
nat_snic_L = nat_sorted_data['L_n*.txt'].tolist()

# Read REM data
rem_filtered_data = data[data['Index'].str.contains('rem_b0')]
rem_filtered_data['sort_key'] = rem_filtered_data['Index'].apply(lambda x: int(x.split('_r')[2]))
rem_sorted_data = rem_filtered_data.sort_values(by='sort_key')
rem_sorted_data = rem_sorted_data.drop(columns=['sort_key'])

rem_host_T = rem_sorted_data['T_h*.txt'].tolist()
rem_host_L = rem_sorted_data['L_h*.txt'].tolist()
rem_snic_T = rem_sorted_data['T_n*.txt'].tolist()
rem_snic_L = rem_sorted_data['L_n*.txt'].tolist()

# Create a figure
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(9, 3), sharey=True)
plt.rcParams['font.size'] = '14'

ax1.plot(traffic_rate,rem_host_T,'o-',color = pal[1],markersize='8', markerfacecolor='white', label="Host Throughput")
ax1.plot(traffic_rate,rem_snic_T,'o-',color = pal[3],markersize='8', markerfacecolor='white', label="SNIC Throughput")

ax1.set_xlabel("Traffic Rate (Gb/s)")
ax1.set_ylabel("Throughput (Gb/s)")
ax1.set_ylim(-10,105)
ax1.yaxis.grid()

# create second y axis
ax11 = ax1.twinx()

ax11.plot(traffic_rate, rem_host_L, 's-', markersize='8', label='Host p99 Latency',  color=pal[1])
ax11.plot(traffic_rate, rem_snic_L, 's--', markersize='8', label='SNIC p99 Latency', color=pal[3])

ax11.set_ylim(-100,2000)
ax11.axes.yaxis.set_ticklabels([])
ax11.text(0.09, 0.92, 'REM', transform=ax11.transAxes, va='center', ha='center', fontsize=14)


## right part: NAT throughput and latency
ax2.plot(traffic_rate,nat_host_T,'o-',color = pal[1], markerfacecolor='white',markersize='8')
ax2.plot(traffic_rate,nat_snic_T,'o-',color = pal[3], markerfacecolor='white',markersize='8')

ax2.set_xlabel("Traffic Rate (Gb/s)")
ax2.set_ylim(-10,105)
ax2.yaxis.grid()

# create second y axis
ax22 = ax2.twinx()

ax22.plot(traffic_rate, nat_host_L, 's-', markersize='8', color=pal[1])
ax22.plot(traffic_rate, nat_snic_L, 's--', markersize='8', color=pal[3])

ax22.set_ylim(-100,2000)
ax22.set_ylabel("p99 Latency (us)")
ax22.text(0.09, 0.92, 'NAT', transform=ax22.transAxes, va='center', ha='center', fontsize=14)


fig.legend(loc='upper center', ncol=2, bbox_to_anchor=(0.48, 1.18), frameon=False)
fig.tight_layout()
plt.savefig("fig4_a.png", bbox_inches='tight')