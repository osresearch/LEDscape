Introduction
============

This is a cookbook for setting up LEDscape on an Ubuntu image (13.10), which boots
from SD.


Make the SD card image
======================

These instructions are based on the Ubuntu Saucy 13.10 Hard Float Minimal Image,
from [armhf](http://www.armhf.com/index.php/boards/beaglebone-black/), and assume
you are using OS X:

	curl http://s3.armhf.com/debian/saucy/bone/ubuntu-saucy-13.10-armhf-3.8.13-bone30.img.xz > ubuntu-saucy-13.10-armhf-3.8.13-bone30.img.xz
	xz -d ubuntu-saucy-13.10-armhf-3.8.13-bone30.img.xz
	sudo dd if=ubuntu-saucy-13.10-armhf-3.8.13-bone30.img of=/dev/diskXX bs=1m

note: Replace diskXX with the device corresponding to your SD card.

First Boot
==========

Once you are set up, it's fine to log in over Ethernet or the usb gadget device, but
for the first boot, it's easiest to connect to it using an FTDI adapter:

	screen /dev/tty.usb* 115200

Username and password are ubuntu/ubuntu.

note: If you use internet sharing on OS X to connect to the device over ethernet,
run grep bootpd /var/log/system.log to figure out what IP address was assigned to
the board.


Install the build tools
=======================

The LEDscape repository is currently source-only, so you'll need to set up a build
environment on your BBB in order to compile it:

	sudo apt-get update
	sudo apt-get install git build-essential device-tree-compiler vim

note: You'll need a working internet connection on the board for this and the
following steps.


Sidenote: Fixing a missing uImage
=================================

At some point after doing a sudo apt-get upgrade, my uImage mysteriously disappeared.
I did the following to rebuild it (note that you need to boot to Angstrom on the mmc
and copy over the uImage from it to the SD card in order to boot Ubuntu to perform this
operation):

	cd /boot 
	sudo apt-get install u-boot-tools
	sudo mkimage -A arm -O linux -T kernel -C none -a 80008000 -e 80008000 -n Linux -d zImage  uImage


Set up Git/GitHub
=================

Do this to get this board authorized for push access to GitHub:

	cd ~
	mkdir .ssh
	ssh-keygen -t rsa -C "email@domain.com"
	(add key to github: https://github.com/settings/ssh)
	git config --global user.email "email@domain.com"
	git config --global user.name "Your Name"


Build LEDscape
==============
Grab the repsitory and build it:

	cd ~
	git clone git@github.com:Blinkinlabs/LEDscape.git
	cd LEDscape
	make

Patch the DTS file (TODO: does the cape manager work properly now?):

	cd dts

	dtc \
	    -I dtb \
	    -O dts \
	    -o ubuntu-`uname -r`.dts \
	    /boot/dtbs/am335x-boneblack.dtb
	(open new dts file, search for 'pruss' section and change 'disabled' to 'okay')

	sudo dtc \
	    -O dtb \
	    -I dts \
	    -o /boot/dtbs/am335x-boneblack.dtb \
	    ubuntu-`uname -r`.dts

And reboot to apply the board configuration change:

	sudo reboot


Use it!
=======

Plug in some LEDs and test it out!

	cd ~/LEDscape/c-examples
	sudo ./matrix-test
