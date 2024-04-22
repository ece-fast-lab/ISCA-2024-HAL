#!/bin/bash

# Set server
COMMAND_SSH="cd ~ && echo $PASSWORD | sudo -S ifconfig enp3s0f0s0 up && sudo -S ifconfig enp3s0f1s0 down"
SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $SNIC_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}
$cmd

# Set SNIC
COMMAND_SSH="cd ~ && echo $PASSWORD | sudo -S ifconfig ens5f0np0 up && sudo -S ifconfig ens5f1np1 down"
SSH_CMD="ssh -i /home/$CLIENT_ACCOUNT/.ssh/id_rsa -F /home/$CLIENT_ACCOUNT/.ssh/config -t $HOST_SSH_IP $COMMAND_SSH"
cmd=${SSH_CMD}
$cmd

# Set client
sudo ifconfig ens1f0np0 up
sudo ifconfig ens1f1np1 down
sleep 2

# check connection
ping -c 3 $HOST_IP
ping -c 3 $SNIC_IP