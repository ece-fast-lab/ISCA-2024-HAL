import matplotlib.pyplot as plt
import argparse

def mk_groups(data):
    try:
        newdata = data.items()
    except:
        return

    thisgroup = []
    groups = []
    for key, value in newdata:
        newgroups = mk_groups(value)
        if newgroups is None:
            thisgroup.append((key, value))
        else:
            thisgroup.append((key, len(newgroups[-1])))
            if groups:
                groups = [g + n for n, g in zip(newgroups, groups)]
            else:
                groups = newgroups
    return [thisgroup] + groups

def add_line(ax, xpos, ypos):
    line = plt.Line2D([xpos, xpos], [ypos + .1, ypos],
                      transform=ax.transAxes, color='black')
    line.set_clip_on(False)
    ax.add_line(line)

def label_group_bar(ax, data):
    groups = mk_groups(data)
    xy = groups.pop()
    x, y = zip(*xy)
    ly = len(y)
    xticks = range(1, ly + 1)
    
    width=0.4
    y1 = [y[i][0] for i in range(ly)]
    y3 = [y[i][1] for i in range(ly) if len(y[i]) > 1]
    y4 = [y[i][2] for i in range(ly) if len(y[i]) > 2]
    y5 = [y[i][3] for i in range(ly) if len(y[i]) > 3]
    y6 = [y[i][4] for i in range(ly) if len(y[i]) > 4]
    
    xticks_1 = [xticks[i] - width for i in range(len(xticks))]
    xticks_3 = [xticks[i] for i in range(ly) if len(y[i]) > 2]
    
    
    lns1 = ax.bar(xticks_1, y3, width=width, color="#009BCB", align='edge', label='Host Power (Host Processing)')
    lns2 = ax.bar(xticks_1, y4, bottom=y3, width=width, color="#5BC1C3", align='edge', label='NIC Power (Host Processing)')
    lns3 = ax.bar(xticks, y5, width=width, color="#F89246", align='edge', label='Host Power (SNIC Processing)')
    lns4 = ax.bar(xticks, y6, bottom=y5, width=width, color="#FFC000", align='edge',  label='NIC Power (SNIC Processing)')
    

    ax2 = ax.twinx()
    
    energy_line_list = [2, 3, 2, 3, 2, 2, 2, 2, 2, 2]
    
    start = 0
    for interval in energy_line_list:
        if start == 0:
            ax2.plot(xticks_3[start:start+interval], y1[start:start+interval], 'o-',markersize='8',markeredgewidth=1.5,markerfacecolor='white', color='#1A638A', label = "Energy Efficiency")
        else:
            ax2.plot(xticks_3[start:start+interval], y1[start:start+interval], 'o-',markersize='8',markeredgewidth=1.5,markerfacecolor='white', color='#1A638A')
        start += interval
        
    for a,b in zip(xticks_3,y1):
        if b > 4.5:
            plt.text(a, 4.5+0.3, '%.1f' % b, ha='center', va= 'bottom',fontsize=11)
        elif b < 0.1:
            plt.text(a, b+0.3, '%.2f' % b, ha='center', va= 'bottom',fontsize=11)
        else:
            plt.text(a, b+0.3, '%.1f' % b, ha='center', va= 'bottom',fontsize=11)

    ax.set_xticks(xticks)
    ax.set_xticklabels(x)
    ax.set_xlim(.5, ly + .5)
    ax.set_ylim(130, 530)
    ax.set_ylabel("Power (W)")
    ax2.set_ylabel("Normalized Energy Efficiency")
    ax2.set_ylim(-8,6)
    ax2.set_yticks([0,1,2,3,4])
    ax2.yaxis.grid()
    ax.legend(loc='upper left', frameon=False)
    ax2.legend(loc='upper right', frameon=False)


    scale = 1. / ly
    for pos in range(ly + 1):
        add_line(ax, pos * scale, -.1)
    ypos = -.2
    while groups:
        group = groups.pop()
        pos = 0
        for label, rpos in group:
            lxpos = (pos + .5 * rpos) * scale
            ax.text(lxpos, ypos, label, ha='center', transform=ax.transAxes)
            add_line(ax, pos * scale, ypos)
            pos += rpos
        add_line(ax, pos * scale, ypos)
        ypos -= .1

if __name__ == '__main__':
    import pandas as pd

    # Set up argument parser
    parser = argparse.ArgumentParser(description='Process the file path.')
    parser.add_argument('file_path', type=str, help='Path to the CSV file')

    # Parse the arguments
    args = parser.parse_args()
    file_path = args.file_path

    # Load the CSV file
    data = pd.read_csv(file_path)

    # Calculate the ratios
    data['T_ratio'] = data['T_n*.txt'] / data['T_h*.txt'] 
    # data['P_ratio'] = (data['P_n*.txt'] - 117) / (data['P_h*.txt'] - 117)
    data['P_ratio'] = data['P_n*.txt'] / data['P_h*.txt']
    
    # Define the key structure
    P_data_structure = {
        'Hardware-acceleratable Function': {
            'REM': ['tea', 'lite'],
            'Crypto': ['RSA', 'DH', 'DSA'],
            'Compress': ['C', 'D']
        },
        'Software-only Function': {
            'KVS': ['R', 'W', 'I'],
            'Count': ['4', '8'],
            'EMA': ['4', '8'],
            'NAT': ['1K', '10K'],
            'BM25': ['2K', '4K'],
            'KNN': ['8', '16'],
            'Bayes': ['128', '256']
        }
    }

    # Define the common value
    common_value = (1, 200, 29, 165, 32)

    # Function to populate the common value for all lowest-level keys
    def populate_values(structure, value):
        return {category: {func: {key: value for key in keys} 
                        for func, keys in functions.items()}
                for category, functions in structure.items()}

    # Populate P_data with the common value
    P_data = populate_values(P_data_structure, common_value)

    # Complete mapping including BM25 and KVS
    complete_mapping = {
        'a0b2000': ('BM25', '2K'),
        'a0b4000': ('BM25', '4K'),
        'a1b128': ('Bayes', '128'),
        'a1b256': ('Bayes', '256'),
        'a2b8': ('KNN', '8'),
        'a2b16': ('KNN', '16'),
        'a3b1000': ('NAT', '1K'),
        'a3b10000': ('NAT', '10K'),
        'a4b1': ('KVS', 'R'),
        'a4b2': ('KVS', 'W'),
        'a4b3': ('KVS', 'I'),
        'a5b4': ('Count', '4'),
        'a5b8': ('Count', '8'),
        'a6b4': ('EMA', '4'),
        'a6b8': ('EMA', '8'),
        'compressb0' : ('Compress', 'C'),
        'compressb1' : ('Compress', 'D'),
        'cyrptob0' : ('Crypto', 'RSA'),
        'cyrptob1' : ('Crypto', 'DH'),
        'cyrptob2' : ('Crypto', 'DSA'),
        'remb0' : ('REM', 'tea'),
        'remb1' : ('REM', 'lite')
    }

    # Update P_data based on the detailed rows of the CSV file for the complete mapping
    for key, (function_name, sub_key) in complete_mapping.items():
        row_index = list(complete_mapping.keys()).index(key)  # Use the index in the mapping as the row index in CSV
        row = data.iloc[row_index]
        T_ratio = row['T_ratio']
        P_ratio = row['P_ratio']
        first_value = T_ratio / P_ratio
        second_value = row['P_h*.txt'] - 29 - 117
        fifth_value = row['P_n*.txt'] - 165 - 117 

        # Determine if the function is hardware-acceleratable or software-only
        if function_name in P_data['Hardware-acceleratable Function']:
            category = 'Hardware-acceleratable Function'
        elif function_name in P_data['Software-only Function']:
            category = 'Software-only Function'
        else:
            continue  # Skip if the function does not exist in either category
        
        P_data[category][function_name][sub_key] = (first_value, second_value, 29, 165, fifth_value)
    
    fig = plt.figure(figsize=(20,4.5))
    plt.rcParams['font.size'] = '14'
    ax = fig.add_subplot(1,1,1)
    label_group_bar(ax, P_data)
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width , box.height* 0.9])
    ax.legend(loc='center', bbox_to_anchor=(0.5, 1.09),ncol=4, frameon=False)
    fig.subplots_adjust(bottom=0.3)
    plt.savefig("fig3.png", bbox_inches='tight')