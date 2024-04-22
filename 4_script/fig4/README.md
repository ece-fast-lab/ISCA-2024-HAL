# Figure 4
1. Environment Setup
```bash
source ../common/env_setup.sh
bash ../common/check_connection.sh
```

2. Run Experiment
```bash
bash run_fig4_left.sh
bash run_fig4_right.sh
```

3. Process Experiment Result
```bash
python3 ../common/process_result.py results results/output.csv
```

4. Draw Figure
```bash
python3 draw_fig4_a.py results/output.csv
python3 draw_fig4_b.py results/output.csv
```

5. View Figure
```bash
feh fig4_a.png
feh fig4_b.png
```