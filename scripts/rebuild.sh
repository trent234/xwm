#!/bin/bash
set -e

HOST_IP=$(ip route | awk '/default/ {print $3}')
echo "HOST_IP=$HOST_IP" > ~/.xwm_env

cd /home/vagrant/xwm
make clean install

# Restart X to re-trigger .xinitrc (and gdbserver)
pkill X
