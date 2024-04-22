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

traffic_rate_P = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100]

traffic_rate_E = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]

# Read NAT data
nat_filtered_data = data[data['Index'].str.contains('a3_b1000')]
nat_filtered_data['sort_key'] = nat_filtered_data['Index'].apply(lambda x: int(x.split('_r')[1]))
nat_sorted_data = nat_filtered_data.sort_values(by='sort_key')
nat_sorted_data = nat_sorted_data.drop(columns=['sort_key'])

nat_host_power = nat_sorted_data['P_h*.txt'].tolist()
nat_host_power = [x - 117 for x in nat_host_power]
nat_host_throughput = nat_sorted_data['T_h*.txt'].tolist()
nat_host_throughput = [x*1500*8/1000 for x in nat_host_throughput]

nat_snic_power = nat_sorted_data['P_n*.txt'].tolist()
nat_snic_power = [x - 117 for x in nat_snic_power]
nat_snic_throughput = nat_sorted_data['T_n*.txt'].tolist()
nat_snic_throughput = [x*1500*8/1000 for x in nat_snic_throughput]

nat_host_efficiency = [ x / y * 1000 for x, y in zip(nat_host_throughput, nat_host_power)]
nat_snic_efficiency = [ x / y * 1000 for x, y in zip(nat_snic_throughput, nat_snic_power)]
nat_host_power = [nat_host_power[0]] + nat_host_power
nat_snic_power = [nat_snic_power[0]] + nat_snic_power

# Read REM data
rem_filtered_data = data[data['Index'].str.contains('rem_b0')]
rem_filtered_data['sort_key'] = rem_filtered_data['Index'].apply(lambda x: int(x.split('_r')[2]))
rem_sorted_data = rem_filtered_data.sort_values(by='sort_key')
rem_sorted_data = rem_sorted_data.drop(columns=['sort_key'])

rem_host_power = rem_sorted_data['P_h*.txt'].tolist()
rem_host_power = [x - 117 for x in rem_host_power]
rem_host_throughput = rem_sorted_data['T_h*.txt'].tolist()

rem_snic_power = rem_sorted_data['P_n*.txt'].tolist()
rem_snic_power = [x - 117 for x in rem_snic_power]
rem_snic_throughput = rem_sorted_data['T_n*.txt'].tolist()

rem_host_efficiency = [ x / y * 1000 for x, y in zip(rem_host_throughput, rem_host_power)]
rem_snic_efficiency = [ x / y * 1000 for x, y in zip(rem_snic_throughput, rem_snic_power)]
rem_host_power = [rem_host_power[0]] + rem_host_power
rem_snic_power = [rem_snic_power[0]] + rem_snic_power


# Create a figure with two subplots
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(9, 3), sharey=True)
plt.rcParams['font.size'] = '14'

# Define a color palette
pal = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']

# Plotting for REM
ax1.plot(traffic_rate_P, rem_host_power, 'o-', color=pal[0], markersize='8', markerfacecolor='white', label="Host Power")
a = 4
ax1.plot(traffic_rate_P[:a], rem_snic_power[:a], 'o-', color=pal[2], markersize='8', markerfacecolor='white', label="SNIC Power")
ax1.plot(traffic_rate_P[a-1:], rem_snic_power[a-1:], 'o--', color=pal[2], markersize='8', markerfacecolor='white')
ax1.set_xlabel("Traffic Rate (Gb/s)")
ax1.set_ylabel("Power (W)")
ax1.set_ylim(100, 400)
ax1.yaxis.grid()

ax11 = ax1.twinx()
ax11.plot(traffic_rate_E, rem_host_efficiency, 's-', color=pal[0], markersize='8', label="Host Energy Efficiency")
a = 4
ax11.plot(traffic_rate_E[:a], rem_snic_efficiency[:a], 's-', color=pal[2], markersize='8', label="SNIC Energy Efficiency")
ax11.plot(traffic_rate_E[a-1:], rem_snic_efficiency[a-1:], 's--', color=pal[2], markersize='8')

ax11.set_ylim(0, 400)  # Adjust as per your data scale
ax11.axes.yaxis.set_ticklabels([])
ax11.text(0.09, 0.92, 'REM', transform=ax11.transAxes, va='center', ha='center', fontsize=14)

# Plotting for NAT
ax2.plot(traffic_rate_P, nat_host_power, 'o-', color=pal[0], markersize='8', markerfacecolor='white')
a = 5
ax2.plot(traffic_rate_P[:a], nat_snic_power[:a], 'o-', color=pal[2], markersize='8', markerfacecolor='white')
ax2.plot(traffic_rate_P[a-1:], nat_snic_power[a-1:], 'o--', color=pal[2], markersize='8', markerfacecolor='white')
ax2.set_xlabel("Traffic Rate (Gb/s)")
ax2.set_ylim(100, 400)
ax2.yaxis.grid()

ax22 = ax2.twinx()
ax22.plot(traffic_rate_E, nat_host_efficiency, 's-', color=pal[0], markersize='8')
a = 5
ax22.plot(traffic_rate_E[:a], nat_snic_efficiency[:a], 's-', color=pal[2], markersize='8')
ax22.plot(traffic_rate_E[a-1:], nat_snic_efficiency[a-1:], 's--', color=pal[2], markersize='8')
ax22.set_ylabel("Energy Efficiency (Mb/J)")
ax22.set_ylim(0, 400)  # Adjust as per your data scale
ax22.text(0.09, 0.92, 'NAT', transform=ax22.transAxes, va='center', ha='center', fontsize=14)

# Legend and Layout Adjustments
fig.legend(loc='upper center', ncol=2, bbox_to_anchor=(0.5, 1.15), frameon=False)

# Adjust the layout
plt.tight_layout()

# Save and Show the plot
plt.savefig("fig4_b.png", bbox_inches='tight')