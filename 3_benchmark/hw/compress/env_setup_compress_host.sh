#!/bin/bash

# enable hugepages
echo "1024" | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
sudo rmmod usdm_drv
sudo insmod /QAT/build/usdm_drv.ko max_huge_pages=8192 max_huge_pages_per_process=384

# update QAT configuration
sudo cp /QATzip/test/performance_tests/config_file/c6xx/c6xx_dev0.conf /etc/c6xx_dev0.conf
sudo cp /QATzip/test/performance_tests/config_file/c6xx/c6xx_dev1.conf /etc/c6xx_dev1.conf
sudo cp /QATzip/test/performance_tests/config_file/c6xx/c6xx_dev2.conf /etc/c6xx_dev2.conf

# restart QAT service
sudo service qat_service restart