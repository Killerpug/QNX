#!/bin/sh

echo "---> Starting serial"
devc-ser8250 -e -b115200

echo "---> Starting USB"
io-usb-otg -d hcd-ehci -d hcd-uhci

#--------------Start networking for VMware Systems-----
echo "---> Starting Networking"
io-pkt-v6-hc -d vmxnet3

if_up -p vx0
ifconfig vx0 up
dhclient -nw vx0
# to manually configure the network address, comment out the dhclient line above and use the line below
#ifconfig vx0 198.x.x.x up

# for VBox networking
#if_up -p wm0
#ifconfig wm0 up
#dhclient -nw wm0

echo "---> Starting Pseudo-tty"

devc-pty

echo "---> Starting qconn Services"
qconn

echo "----> Completed /etc/startup_vmware.sh script"
