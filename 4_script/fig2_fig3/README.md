# Figure 2 and Figure 3

1. Environment Setup
```bash
source ../common/env_setup.sh
bash ../common/check_connection.sh
```

2. Run Experiment
```bash
bash run_fig2_fig3_sw.sh
bash run_fig2_fig3_hw.sh
```

3. Process Experiment Result
```bash
python3 ../common/process_result.py results results/output.csv
```

4. Draw Figure
```bash
python3 draw_fig2.py results/output.csv
python3 draw_fig3.py results/output.csv
```

5. View Figure
```bash
feh fig2.png
feh fig3.png
```