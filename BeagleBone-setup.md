Building
--------

SSL install is broken; [http://derekmolloy.ie/fixing-git-and-curl-certificates-problem-on-beaglebone-blac/](disable certs):

    git config --global http.sslVerify false

Checkout LEDscape and build both PRU and teensy LEDscape code.

    git clone https://osresearch@github.com/osresearch/LEDscape
    cd LEDscape
    make


Teensy config
-------------

* Code is in `LEDscape/VideoDisplay/`
* Update `LED_WIDTH` to have the maximum length of the strips
* Rebuild and flash to all teensys (do this on the laptop)

* Find the teensy serial IDs with the `find-serial` tool in LEDscape:

    root@beaglebone:~/LEDscape# ./find-serial 
    /dev/ttyACM0: 8998
    /dev/ttyACM1: 14401
    /dev/ttyACM2: 14389
    /dev/ttyACM3: 8987

* Update `config.txt`.  At the minimum it needs the first line with the
maximum strip length and the UDP port number, and four lines for the
teensy serial IDs and their Y-offset in the image buffer.  It is ok
to have teensys that are not in use with this BBB so that the same
config file can be used with all of them.

    10,9999
    14401,0,
    14389,8,
    8987,16,1:9,0:8
    8998,24,

* If there are any bad pixels, they go on the line with that ID.  In the
above example, there are two bad pixels, both on the same teensy.
Pixel #9 on strip #1 and pixel #8 on strip #0 are marked as "bad" and
will not be set to anything other than black.

* Run `teensy-rx-udp` on the BeagleBone.  It should report opening all
four serial ports and querying them to find out their configured pixel
lengths (which *must match the config file*):

    root@beaglebone:~/LEDscape# ./teensy-udp-rx  config.txt
    8987: bad 1:9
    8987: bad 0:8
    /dev/ttyACM0: ID 8998 width 10
    /dev/ttyACM1: ID 14401 width 10
    /dev/ttyACM2: ID 14389 width 10
    /dev/ttyACM3: ID 8987 width 10

* If a device is cleanly unplugged and replugged, it should report:

    teensy_open:286: /dev/ttyACM1: read failed: Resource temporarily unavailable
    main:508: /dev/ttyACM1: write failed: No such device
    /dev/ttyACM1: ID 14401 width 10

USB problems
------------

* USB devices don't show up when the teensy is not plugged in during startup.
* Clean disconnect from HUB is OK.
* Power kill to the teensys and reconnect is not:

    [  243.372796] musb_stage0_irq 496: bogus host RESUME (b_peripheral)

* power management seems to just hose the port.
* but....  `lsusb -v` seems to bring them back?
* Solution:

	while true; do
		lsusb -v > /dev/null
		sleep 5
	done

