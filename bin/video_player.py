#!/usr/bin/python
# Play a video onto the screen

import cv2, cv
import socket
import numpy
import datetime
import time
import argparse

parser = argparse.ArgumentParser(
    description="Display a video on the LEDscape screen"
    )
parser.add_argument(
    "-s", "--screenGeometry",
    dest="screenGeometry",
    help="LEDscape screen size (ex: 256x32)",
    default="256x32",
    )
parser.add_argument(
    "-a", "--address",
    dest="address",
    help="TCP address to connect to (ex: localhost)",
    default="localhost",
    )
parser.add_argument(
    "-p", "--port",
    dest="port",
    help="TCP port to send data to (ex: 9999)",
    default=9999,
    type=int,
    )
parser.add_argument(
    "-l", "--loop",
    action="store_true",
    dest="loop",
    help="Loop the video continuously",
    default=False,
    )
parser.add_argument(
    dest="filename",
    help="Filename to load",
    )    

config = parser.parse_args()

# LEDscape screen geometry
screenWidth = int(config.screenGeometry.split('x')[0])
screenHeight = int(config.screenGeometry.split('x')[1])

# If the screen height is not a divisor of 2, then the subframe scheme won't work.
if screenHeight % 2:
    print "Error, screen height must be a multiple of 2!"
    exit(1)

# LEDscape packet geometry
# Note: SubframeCount *must* be an integer divider of screenHeight!!
# Note: This is designed for large screen support, a la the megasidescroller
# Small screens only need one frame to transmit, so set subFrameCount to 1.
subFrameCount = 2
subFrameHeight = screenHeight / subFrameCount
subFrameSize = 1 + subFrameHeight*screenWidth*3

# LEDscape message setup
message = numpy.zeros(subFrameSize*subFrameCount, numpy.uint8);
for subFrame in range(0, subFrameCount):
    message[subFrame*subFrameSize] = subFrame

# Socket to send to
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET,socket.SO_SNDBUF,int(subFrameSize))

# Test if the frame size is acceptable
# TODO: Fix this
if sock.getsockopt(socket.SOL_SOCKET,socket.SO_SNDBUF) < subFrameSize:
    print "Error configuring TCP socket: buffer too big (reduce LEDscape image size?)"
    exit(1)

# Open the video for playback
cap = cv2.VideoCapture(config.filename)


# Test that the video was loaded
if not cap.isOpened():
    print "Error opening video: Check that the file exists and the format is correct"
    exit(1)

fps = cap.get(cv.CV_CAP_PROP_FPS)
frameDelay = 1.0/fps

nextTime = time.time() + frameDelay

while cap.isOpened():
    # Get the video frame
    ret, frame = cap.read()

    # If we've reached the end, reset the position to the beginning
    if not ret:
        if config.loop:
            cap.set(cv.CV_CAP_PROP_POS_MSEC, 0)
            ret, frame = cap.read()
        else:
            exit(0)

    # Resize the video to be the width that we actually want
    originalHeight = frame.shape[0]
    originalWidth = frame.shape[1]
    originalAspect = float(originalWidth)/originalHeight
    
    desiredWidth = 32*5
    desiredHeight = int(desiredWidth/originalAspect)

    smaller = cv2.resize(frame,(desiredWidth, desiredHeight))
    frame = smaller

    # Copy the image data into the LEDscape format
    frameHeight = frame.shape[0]
    frameWidth = frame.shape[1]

    flattenedFrame = frame.reshape(frameHeight, frameWidth*3)

    copyWidth = min(screenWidth, frameWidth)
    copyHeight = min(screenHeight, frameHeight)

    copyLength = copyWidth*3

    for row in range(0, copyHeight):
        offset = 1 + (row / subFrameHeight)
        messageOffset = (row*screenWidth)*3 + offset

        message[messageOffset:messageOffset+copyLength] = flattenedFrame[row, 0:copyLength]

    # Send the data to the LEDscape host
    for subFrame in range(0, subFrameCount):
        sock.sendto(message[subFrame*subFrameSize:(subFrame+1)*subFrameSize], (config.address, config.port))

    # Delay until it's time to show the next frame.
    while time.time() < nextTime:
       pass

    nextTime += frameDelay

