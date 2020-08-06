#!/bin/ash

# Push to Server, written by Michael Bernhard
# Necessary for watering system to upload log files and threshold values from SD card to server
#
# copy this file to Arduino Yun file system "/mnt/sd/watering/pushToServer.sh". E.g. by using "scp" command.
#
# change place holders
# <ftp_server_name>: e.g. ftp://ft_server_name/root_path
# <username>
# <password>

if [ ! "$1" = "" ]; then
  curl -T /mnt/sd/watering/$1 <ftp_server_name>/$1 --user <username>:<password>
fi

