#!/bin/bash

cp ip/* HAL-FPGA/fpga/mqnic/AU280/fpga_100g/ip/
cp HAL-FPGA/fpga/lib/axis/rtl/axis_register.v HAL-FPGA/fpga/lib/axis/rtl/feedback_axis_register.v
cp HAL-FPGA/fpga/lib/axis/rtl/axis_register.v HAL-FPGA/fpga/lib/axis/rtl/director_axis_register.v
for patch_file in patch/*.patch; do
    patch -p0 < $patch_file
done