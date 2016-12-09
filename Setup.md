# Getting Started

This is a quick introduction on how to set up LEDscape on a Debian-based image

# Setting up the BBB environment

To develop for LEDscape on a Debian environment, Start by copying the latest BBB image to an SD card. These instructions were made using:

    BBB-eMMC-flasher-debian-7.8-console-armhf-2015-03-01-2gb.img.xz

The latest version can be found at:

    http://elinux.org/Beagleboard:BeagleBoneBlack_Debian#Debian_Releases

First, we need to expand the image to use the whole SD card. By default, it is only 2GB.

Note: If using a RevC or lower BBB, the EMMC is only 2GB so this isn't necessicary.

    cd /opt/scripts/tools/
    sudo ./grow_partition.sh
    sudo reboot

Next, update the the Debian environment:

    sudo apt-get update
    sudo apt-get install usbmount -y
    sudo apt-get install git build-essential python vim -y
    
Disable the HDMI output:

If you are using a Debian image from 2014.8.13 or newer, do this:

    sudo cp /boot/uEnv.txt /boot/uEnv.txt.backup
    sudo sed -i 's/#cape_disable=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN$/cape_disable=capemgr.disable_partno=BB-BONELT-HDMI,BB-BONELT-HDMIN/g' /boot/uEnv.txt


Otherwise, modify the uEnv boot file by hand to disable HDMI and HDMIN overlays.

Enable a larger shared memory space (8MB) with the PRU:

	echo 'options uio_pruss extram_pool_sz=0x800000' | sudo tee -a /etc/modprobe.d/ledscape.conf
	
Finally, reboot to apply these changs:

    sudo reboot

# Next, set up LEDscape:

Use git to download the repository:

    cd /opt
    sudo git clone http://github.com/osresearch/LEDscape.git
    cd LEDscape
    sudo make
    
Copy the device tree file into place, and add it to the slots:

    sudo cp dts/CAPE-BONE-OCTO-00A0.dtbo /lib/firmware
    echo 'CAPE-BONE-OCTO' | sudo tee -a /sys/devices/bone_capemgr.*/slots
    
Then run the identification program to test if all is well:

    sudo bin/identify

# Make a configuration file for your screen

The configuration file is what tells LEDscape how to draw it's virtual screen onto your matrix tiles or LED strips. There are two basic formats:

## Matrix screen

Let's look at a sample matrix configuration. Here's one for a small display consisting of 4 LED matricies, arranged in a square:

    matrix16
    0,6 N 0,0
    0,7 N 32,0
    1,6 N 0,16
    2,7 N 32,16
    
The first line of the configuration file describes the type of matrix. Here are the valid matrix types:

| Type      | Description           |
| ------------- |:-------------:|
| matrix16      | 32x16 LED matrix, 1/8th scanning (three address pins) |

Each following line consists of three sets of information: Controller position, Rotation, and LEDscape virtual screen offset.

The controller position consists of two values. The first is the output channel. This corresponds to the physical output that the matrix is plugged into on the board. There are 8 outputs available on the OCTOscroller shield. The second is the position in the output chain. This one is a little more tricky, as it is backwards from you might expect. The *first* matrix panel in the chain, which is the one connected to the OCTOscroller shield, is called 7. The second one is called 6, and so on, until the eighth and final one. For example, a single matrix panel connected to output #3 has the following controller position:

    3,7
    
The rotation is any one of the following values: N, U, L, R. 'N' and 'U' are used when the long side of the panel is parallel to the ground, and 'L' and 'R' are used when it is perpendicular. Try one, then the other, to figure out the correct orientation for your panels.

The Virtual screen offset is the top-left position in the LEDscape virtual screen that will be drawn to this matrix panel. Normally you will want to map sections of the screen into contigouos regions, so the top-left panel in your display should have a virtual screen offset of 0,0, then the panel to the right of that one should be offset by the width of the first panel, either 16,0 or 32,0, and so on.


## WS2812 strips

Let's look at a sample WS2812 strip configuration. Here's one that can control a single strip output:

    ws2812
    64,48

The first line of the configuration file describes the type of matrix. Here are the valid matrix types:

| Type          | Description           |
| ------------- |:-------------:|
| ws2812        | Strip of WS2812/WS2812B LEDs, aka NeoPixels |

The following line consists of Width,Height. For LED strip mode, this means:

W: length of the longest strip
H: number of output strips

## Testing your configuration

For matricies, there is a handy identification program to draw some text that identifies each panel. Run it to test your new configuration:

    sudo bin/identify myconfig.config


# Set up the UDP listener to display incoming packets
    
To run the matrix listener:
    
    sudo bin/matrix-udp-rx -W 256 -H 32 -c sign.config &

Or for WS2812B strips:

    sudo bin/matrix-udp-rx -W 256 -H 32 -c strips.config &   
 
There are a bunch of command line arguments, and the whole thing seems to be in a bit of flux. Here's what exists now:

| Argument      | Description           | Default |
| ------------- |:-------------:| -----:|
| -v      | Verbose mode |  |
| -n      | Skip LEDscape initialization      | |
| -c      | Configuration file to use. Note: Use full pathname, for example: /home/debian/LEDscape/default.config      | |
| -t      | Number of seconds the display server will show the previous image before timing out      | 60 |
| -W      | LEDscape virtual screen width      | 256 |
| -H      | LEDscape virtual screen height      | 128 |
| -m      | Message to display at startup      | |


# Run the UDP listener automatically at system boot

There's a handy script for starting LEDscape at boot. It should listen on the ethernet interface on port 9999 automatically.

## Debian

Debian uses systemd. Do this:

    sudo cp bin/ledscape.service /etc/systemd/system
    sudo systemctl enable ledscape.service

Extra: for video playback

    sudo cp bin/videoplayer.service /etc/systemd/system
    sudo systemctl enable videoplayer.service

## Ubuntu

Note: The Ubuntu startup script is old and probably needs fixing.

Ubuntu appears to use upstart. Do this:

    sudo cp ubuntu/ledscape.conf /etc/init
    sudo start ledscape

# Extra credit

## Make the filesystem read-only

To help prevent the configuration from breaking, it's helpful to configure the system as read only. The insructions are from:

    http://armsdr.blogspot.com/2014/11/beaglebone-debian-read-only-filesystem.html

First, back up the original fstab:

	sudo cp /etc/fstab /etc/fstab_rw
    
Now, set the root filesystem to be readonly:

	sudo sed -i 's/ext4  noatime/ext4  ro,noatime/g' /etc/fstab
    
Finally, add some temporary rw mounts so that the system can continue to function:

	echo "tmpfs /tmp tmpfs nodev,nosuid,size=32M 0 0
	tmpfs /srv tmpfs nodev,size=512K 0 0
	tmpfs /var/log tmpfs defaults,noatime,size=1M 0 0
	tmpfs /var/tmp tmpfs defaults,noatime,size=512K 0 0
	tmpfs /var/run tmpfs defaults,noatime,size=512K 0 0" | sudo tee -a /etc/fstab


Later, write capability can be restored using the following command:

    sudo mount -o remount,rw /

And disabled again using:

    sudo mount -o remount,ro /

## Speed up boot when Ethernet is not connected

The ethernet PHY on some BBBs can be flaky and cause a long delay during boot. To work around this, set it to not load at boot:

    sudo sudo sed -i 's/auto eth0/#auto eth0/g' /etc/network/interfaces    

Later it can be enabled by typing:

	sudo ifup eth0

## Set the time automatically (when network is available)

	sudo apt-get install ntpdate -y
	

# Video playback

Playing a video is as simple as running the video player (after running the UDP listener):

    bin/video_player -s 256x32 -l ../Daft\ Punk\ -\ Around\ The\ World.avi

To use the video player, some additional packages will need to be installed:
    
    sudo apt-get install libavformat-dev x264 v4l-utils ffmpeg libcv2.3 libcvaux2.3 libhighgui2.3 python-opencv opencv-doc libcv-dev libcvaux-dev libhighgui-dev -y
    
    
#Helpful things for development

## Make VIM prettier

Enable color highlighting

	echo 'syntax on' >>~/.vimrc
	
