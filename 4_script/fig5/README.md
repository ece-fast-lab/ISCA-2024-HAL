# Figure 5
1. Environment Setup
```bash
source ../common/env_setup.sh
bash ../common/check_connection.sh
```

2. Run Experiment
```bash
bash run_fig5.sh
```

3. Process Experiment Result
```bash
python3 ../common/process_result_split.py results results/output.csv
```

4. Draw Figure
```bash
python3 draw_fig5.py results/output.csv
```

5. View Figure
```bash
feh fig5.png
```