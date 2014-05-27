Getting Started
===============

This is a quick introduction on how to set up LEDscape on a Debian-based image

Setting up the BBB environment
==============================

To develop for LEDscape on a Debian environment, Start by copying the latest BBB image to an SD card. These instructions were made using version bone-debian-7.5-2014-05-14-2gb.img. The latest version can be found at:

    http://elinux.org/BeagleBoardDebian#Demo_Image

First, we need to expand the image to use the whole SD card. By default, it is only 2GB.

    cd /opt/scripts/tools/
    sudo ./grow_partition.sh
    sudo reboot

Next, update the the Debian environment:

    sudo apt-get update
    
Disable the HDMI output:

    sudo sed -i 's/#cape_disable=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN/cape_disable=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN'/g /boot/uboot/uEnv.txt
    sudo reboot

Note: These packages used to be required, but now are included in the default image:

    sudo apt-get install git build-essential

Next, set up LEDscape:
======================

Use git to download the repository, then build it:

    git clone http://@github.com:osresearch/LEDscape.git
    cd LEDscape
    make
    
Copy the device tree file into place, and add it to the slots:

    sudo cp dts/CAPE-BONE-OCTO-00A0.dtbo /lib/firmware
    echo 'CAPE-BONE-OCTO' | sudo tee -a /sys/devices/bone_capemgr.9/slots
    
Then run the identification program to test if all is well:

    sudo bin/identify
    
    
Set up the UDP listener to display incoming packets
===================================================
    
To run the matrix listener:
    
    bin/matrix-udp-rx sign.config n/matrix-udp-rx -W 256 -H 32 -c sign.config &
    
There are a bunch of command line arguments, and the whole thing seems to be in a bit of flux.


Video playback
==============

To play back a video, we'll need a couple of additional libraries:

    sudo apt-get install libavformat-dev x264 v4l-utils ffmpeg
    sudo apt-get install libcv2.3 libcvaux2.3 libhighgui2.3 python-opencv opencv-doc libcv-dev libcvaux-dev libhighgui-dev

Then it's as simple as running the video player (after running the UDP listener):

    bin/video_player.py -s 256x32 -l ../Daft\ Punk\ -\ Around\ The\ World.avi
    
