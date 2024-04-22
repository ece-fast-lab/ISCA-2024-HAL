#!/bin/bash

# stop QAT service
sudo service qat_service stop

# update QAT configuration
sudo cp /QAT_Engine/qat_hw_config/c6xx/multi_thread/c6xx_dev0.conf /etc/c6xx_dev0.conf
sudo cp /QAT_Engine/qat_hw_config/c6xx/multi_thread/c6xx_dev0.conf /etc/c6xx_dev1.conf
sudo cp /QAT_Engine/qat_hw_config/c6xx/multi_thread/c6xx_dev0.conf /etc/c6xx_dev2.conf

# start QAT service
sudo service qat_service start