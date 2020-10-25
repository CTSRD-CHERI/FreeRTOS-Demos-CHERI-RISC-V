#!/usr/bin/bash

ip link add br0 type bridge &&
ip tuntap add dev tap0 mode tap &&
ip link set dev tap0 master br0 &&  # set br0 as the target bridge for tap0
ip link set dev eth0 master br0 &&  # set br0 as the target bridge for eth0
ip link set dev br0 up &&
ip route del default &&
ip addr add 10.88.88.1/24 dev br0 &&
ip addr add 10.88.88.3/24 dev eth0 &&
ip route add default via 10.88.88.1 &&
ip route add 10.88.88.2 via 10.88.88.1 &&
ip route add 10.88.88.3 via 10.88.88.1 &&
ip link set dev tap0 up
