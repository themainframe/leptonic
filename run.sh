#!/bin/bash
echo "Copying..."
rsync -rqa . pi@10.10.1.13:/home/pi/leptonic-c
echo "Running..."
ssh pi@10.10.1.13 "cd /home/pi/leptonic-c && make"
# echo "Downloading & converting image..."
# scp pi@10.10.1.13:/home/pi/leptonic-c/IMG_0000.pgm .
# convert IMG_0000.pgm image.png
# open image.png
