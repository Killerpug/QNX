#!/bin/sh

echo "---> Starting serial"
devc-ser8250 -e -b115200

echo "---> Starting USB"
io-usb-otg -d hcd-ehci -d hcd-uhci


echo "---> Starting Networking"
io-pkt-v6-hc -d e1000
if_up -p wm0
ifconfig wm0 up
dhclient -nw wm0

# to manually configure the network address, comment out the dhclient line above and use the line below
#ifconfig wm0 198.x.x.x up


echo "---> Starting Pseudo-tty"

devc-pty

echo "---> Starting qconn Services"
qconn

echo "----> Completed /etc/startup_vbox.sh script"
