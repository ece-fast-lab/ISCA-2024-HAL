import matplotlib.pyplot as plt
import argparse

def mk_groups(data):
    try:
        newdata = data.items()
    except AttributeError:
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
    
    width = 0.4
    y1 = [y[i][0] for i in range(ly)]
    y2 = [y[i][1] for i in range(ly)]
    y3 = [y[i][2] for i in range(ly) if len(y[i]) > 2]
    xticks_1 = [xticks[i] - width for i in range(len(xticks))]
    xticks_3 = [xticks[i] for i in range(ly) if len(y[i]) > 2]
    
    for a, b in zip(xticks_1, y2):  
        if b > 3.0:
            ax.text(a + 0.2, 3.0 + 0.05, '%.1f' % b, ha='center', va='bottom', fontsize=11) 
    
    for a, b in zip(xticks, y1):  
        if b > 3.0:
            ax.text(a + 0.2, 3.0 + 0.05, '%.1f' % b, ha='center', va='bottom', fontsize=11)

    ax.bar(xticks_1, y2, width=width, color='#009BCB', align='edge', label='Throughput of SNIC Processor', zorder=2)
    ax.bar(xticks, y1, width=width, color='#FFC000', align='edge', label='p99 Latency of SNIC Processor', zorder=2)

    ax.set_xticks(xticks)
    ax.set_xticklabels(x)
    ax.set_xlim(.5, ly + .5)
    ax.set_ylim(0, 3.0)
    ax.set_ylabel("Normalized Throughput\nNormalized p99 Latency")
    ax.yaxis.grid(zorder=0)
    ax.axhline(1, color='black', linestyle='--', linewidth=1.5, zorder=1)
        

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
        
def update_value(data, path, factor=1):
    # Unpack the path to get to the specific tuple
    *keys, last_key = path
    # Navigate through the nested dictionary to the desired tuple
    nested_dict = data
    for key in keys:
        nested_dict = nested_dict[key]

    # Extract the old tuple, compute the new value, and create a new tuple
    old_tuple = nested_dict[last_key]
    new_value = (1 / old_tuple[1] * factor,) + old_tuple[1:]

    # Update the dictionary with the new tuple
    nested_dict[last_key] = new_value

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
    data['L_ratio'] = data['L_n*.txt'] / data['L_h*.txt']
    data['T_ratio'] = data['T_n*.txt'] / data['T_h*.txt']
    
    TL_data = {
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

    # Function to reconstruct the original three-layer structure with (1, 1) for each leaf node
    def expand_TL_data(data):
        return {category: {func: {key: (1, 1) for key in keys} for func, keys in functions.items()}
                for category, functions in data.items()}

    TL_data = expand_TL_data(TL_data)


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

    # Update TL_data based on the detailed rows of the CSV file for the complete mapping
    for key, (function_name, sub_key) in complete_mapping.items():
        row_index = list(complete_mapping.keys()).index(key)  # Use the index in the mapping as the row index in CSV
        # Determine if the function is hardware-acceleratable or software-only
        if function_name in TL_data['Hardware-acceleratable Function']:
            category = 'Hardware-acceleratable Function'
        elif function_name in TL_data['Software-only Function']:
            category = 'Software-only Function'
        else:
            continue  # Skip if the function does not exist in either category

        # Update the corresponding entry in TL_data
        TL_data[category][function_name][sub_key] = (
            data.loc[row_index, 'L_ratio'], 
            data.loc[row_index, 'T_ratio']
        )

    # Define the path for each item you want to update
    paths = [
        (['Hardware-acceleratable Function', 'Crypto', 'RSA'], 1),
        (['Hardware-acceleratable Function', 'Crypto', 'DH'], 1),
        (['Hardware-acceleratable Function', 'Crypto', 'DSA'], 1),
        (['Hardware-acceleratable Function', 'Compress', 'C'], 2/3),
        (['Hardware-acceleratable Function', 'Compress', 'D'], 2/3),
    ]

    # Apply the updates
    for path, factor in paths:
        update_value(TL_data, path, factor)

    fig = plt.figure(figsize=(20, 4))
    plt.rcParams['font.size'] = '14'
    ax = fig.add_subplot(1, 1, 1)
    label_group_bar(ax, TL_data)
    fig.subplots_adjust(bottom=0.3)

    ax.legend(loc='center', bbox_to_anchor=(0.5, 1.15),ncol=4, frameon=False)
    plt.savefig("fig2.png", bbox_inches='tight')