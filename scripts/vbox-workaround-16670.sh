#!/bin/bash

# Detect and correct bug in vbox guest additions 5.1.20
# See https://www.virtualbox.org/ticket/16670

[ -e "/sbin/mount.vboxsf" ] || sudo ln -sf /opt/VBoxGuestAdditions-5.1.20/lib/VBoxGuestAdditions/mount.vboxsf /sbin/mount.vboxsf
mount -t vboxsf -o uid=`id -u ubuntu`,gid=`id -g ubuntu` vagrant /vagrant
