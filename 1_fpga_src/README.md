# HAL FPGA Implementation

### Get Corundum source code
```bash
git clone https://github.com/corundum/corundum.git HAL-FPGA
cd HAL-FPGA 
git checkout c708bc4
```

### Apply changes
```bash
cd ..
bash apply_changes.sh
```

### FPGA Compliation
```bash
cd HAL-FPGA/fpga/mqnic/AU280/fpga_100g/fpga
make -j
```
It will take about 4 hours to finish compliation

### FPGA Programming
Program FPGA using Vivado with generated bitstream: `fpga/mqnic/AU280/fpga_100g/fpga/fpga.bit`

### FPGA PCIe Reset
```bash
cd HAL-FPGA
sudo bash fpga/lib/pcie/scripts/pcie_hot_reset.sh <FPGA PCIe address>
```

### Load FPGA Kernel Module 
```bash
cd HAL-FPGA/modules/mqnic
make -j
sudo insmod mqnic.ko
```