import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
import matplotlib.colors as mcolors
from operator import add
import argparse

# Set up argument parser
parser = argparse.ArgumentParser(description='Process the file path.')
parser.add_argument('file_path', type=str, help='Path to the CSV file')

# Parse the arguments
args = parser.parse_args()
file_path = args.file_path


df = pd.read_csv(file_path)
plt.rcParams['font.size'] = '14'

bar_indices = ["_sp_a3_b1000_c1_s{}_r8", "_sp_a3_b1000_c2_s{}_r8", "_sp_a3_b1000_c4_s{}_r8"]
group_names = [str(i*10) for i in range(2, 7)]
bar_labels = ["SLB 1 core", "SLB 2 cores", "SLB 4 cores"]
n_bars = len(bar_labels)
group_positions = np.arange(len(group_names))
bar_width = 0.2
colors = ["#376795", "#72BCD5", "#FFD06F"]
pal = sns.color_palette("Paired")

filtered_df = pd.DataFrame(columns=["Split Traffic Threshold (Gb/s)", "Bar", "Throughput (Gb/s)", "Throughput_Host (Gb/s)", "Work (Gb/s)"])
for group in group_names:
    for bar in bar_indices:
        bar_index = bar.format(str(int(group)//10))
        if any(df['Index'].str.contains(bar_index)):
            row = df[df['Index'].str.contains(bar_index)]
            filtered_df = filtered_df._append({
                "Split Traffic Threshold (Gb/s)": group,
                "Bar": bar.format(str(int(group)//10)),
                "Throughput (Gb/s)": row["T_n_rx"].values[0],
                "Throughput_Host (Gb/s)": row["T_n_tx"].values[0],
                "Work (Gb/s)": row["T_n_work"].values[0]
            }, ignore_index=True)

        
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(8, 3))
for i in range(0, n_bars):
    line_data_work = filtered_df['Work (Gb/s)'][i::n_bars]
    line_data_work = [k * 532 * 8 /1000 for k in line_data_work]

    line_data_host = filtered_df['Throughput_Host (Gb/s)'][i::n_bars]
    line_data_host = [k * 532 * 8 /1000 for k in line_data_host]
    line_data_total = list( map(add, line_data_host, line_data_work) )

    ax1.bar(group_positions - bar_width*(n_bars-1)/2 + i*bar_width, line_data_total, 
            width = bar_width, bottom=0,  
            label=bar_labels[i], color=colors[i], linewidth=0.7, zorder=2)

ax1.set_xlabel('Fwd$_{Th}$  (Gb/s)')
ax1.set_ylabel('Throughput (Gb/s)')
ax1.set_xticks(group_positions)
ax1.set_xticklabels(group_names)
ax1.set_ylim(0, 90)

ax1.axhline(18.80 * 532 * 8 /1000, color=pal[1], linestyle='--', linewidth=1.5, label = "All to Host")
ax1.axhline(15.55 * 532 * 8 /1000, color=pal[3], linestyle='--', linewidth=1.5, label = "All to SNIC")
ax1.yaxis.grid(zorder=0)

filtered_df = pd.DataFrame(columns=["Split Traffic Threshold (Gb/s)", "Bar", "Latency (us)"])
for group in group_names:
    for bar in bar_indices:
        bar_index = bar.format(str(int(group)//10))
        if any(df['Index'].str.contains(bar_index)):
            row = df[df['Index'].str.contains(bar_index)]
            filtered_df = filtered_df._append({
                "Split Traffic Threshold (Gb/s)": group,
                "Bar": bar.format(str(int(group)//10)),
                "Latency (us)": row["L_n"].values[0]
            }, ignore_index=True)

for i in range(0, n_bars):
    line_data = filtered_df['Latency (us)'][i::n_bars]
    ax2.bar(group_positions - bar_width*(n_bars-1)/2 + i*bar_width, line_data, 
            width = bar_width, label=bar_labels[i], 
            color=colors[i], linewidth=0.7, zorder=2)


ax2.set_xlabel('Fwd$_{Th}$ (Gb/s) ')
ax2.set_ylabel('p99 Latency (us)')
ax2.set_xticks(group_positions)
ax2.set_xticklabels(group_names)
ax2.set_ylim(0, 1250)


ax2.axhline(12.29, color=pal[1], linestyle='--', linewidth=1.5, label = "All to Host")
ax2.axhline(357.73, color=pal[3], linestyle='--', linewidth=1.5, label = "All to SNIC")
ax2.yaxis.grid(zorder=0)

lines, labels = ax1.get_legend_handles_labels()
fig.legend(lines, labels, loc='upper center', ncol=3, bbox_to_anchor=(0.5, 1.19), frameon=False)


# Adjust the layout
fig.tight_layout()

# Show the plot
plt.savefig("fig5.png", bbox_inches='tight')