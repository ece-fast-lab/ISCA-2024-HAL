# Figure 9a
1. Environment Setup
```bash
source ../common/env_setup.sh
bash ../common/check_connection_hal.sh
```

2. Run Experiment
```bash
bash run_fig9_a.sh
```

3. Process Experiment Result
```bash
python3 ../common/process_result.py results results/output.csv
```

4. Draw Figure
```bash
python3 draw_fig9_a.py results/output.csv
```

5. View Figure
```bash
feh fig9_a.png
```