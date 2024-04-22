#!/bin/bash

echo "8192" | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

export CLIENT_ACCOUNT=artifact # client user account
export HOST_ACCOUNT=artifact # server user account
export HOST_SSH_IP= # server IP
export SNIC_ACCOUNT=artifact # SNIC user account
export SNIC_SSH_IP= # SNIC IP
export PASSWORD='' # password for the server and SNIC

export CLIENT_IP=192.168.200.51
export CLIENT_MAC= #
export HOST_IP=192.168.200.31
export HOST_MAC=
export SNIC_IP=192.168.200.33
export SNIC_MAC=

export CLIENT_IP_HAL=192.168.200.52
export CLIENT_MAC_HAL= #
export HOST_IP_HAL=192.168.200.32
export HOST_MAC_HAL=
export SNIC_IP_HAL=192.168.200.34
export SNIC_MAC_HAL=

export CLIENT_NIC_PCIE1="0000:02:00.0" #
export CLIENT_NIC_PCIE2="0000:02:00.1" #

export REPO_PATH=/home/artifact/ISCA-2024-HAL # path to the repository