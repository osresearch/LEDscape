Getting Started
===============

This is a quick introduction on how to set up LEDscape on a debian system

Setting up the BBB environment
==============================

To develop for LEDscape on a Debian environment, 

First, we need to install the set up the Debian environment:

    apt-get update
    sudo apt-get install git build-essential

    (disable HDMI capes)

Next, set up LEDscape:
======================

    git clone git@github.com:osresearch/LEDscape.git
    cd LEDscape
    make

To run the matrix listener:

    sudo su
    cd LEDscape
    echo "CAPE-BONE-OCTO" > /sys/devices/bone_capemgr.9/slots
    bin/matrix-udp-rx sign.config n/matrix-udp-rx -W 256 -H 32 -c sign.config&
    

Video playback
==============

To play back a video, we'll need a couple of additional libraries:

    sudo apt-get install libavformat-dev x264 v4l-utils ffmpeg
    sudo apt-get install libcv2.3 libcvaux2.3 libhighgui2.3 python-opencv opencv-doc libcv-dev libcvaux-dev libhighgui-dev

Then it's as simple as running the video player:

    bin/video_player.py -s 256x32 -l ../Daft\ Punk\ -\ Around\ The\ World.avi
    
