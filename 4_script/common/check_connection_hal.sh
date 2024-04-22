#!/bin/bash

# Set server
COMMAND_SSH="cd ~ && echo $PASSWORD | sudo -S ifconfig enp3s0f0s0 down && sudo -S ifconfig enp3s0f1s0 up"
SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $SNIC_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}
$cmd

# Set SNIC
COMMAND_SSH="cd ~ && echo $PASSWORD | sudo -S ifconfig ens5f0np0 down && sudo -S ifconfig ens5f1np1 up"
SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}
$cmd

# Set client
sudo ifconfig ens1f0np0 down
sudo ifconfig ens1f1np1 up
sleep 2

# check connection
ping -c 3 $SNIC_IP_HAL