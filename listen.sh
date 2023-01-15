#!/bin/bash

set -e

if [ $# -lt 2 -o $# -gt 3 ]; then
	echo "Usage: $0 monitor stream-name [auto-connect-pattern]"
	echo ""
	echo "monitor: the binary of the monitor to execute"
	echo "stream-name: the name of the stream to connect to"
	echo "auto-connect-pattern: if the SHM buffer matches this pattern, do not ask whether to connect, do it!"
	exit 1
fi

MON=$1
STREAM=$2
AUTOCONN="$3"

while true; do
	echo -e "\033[33mWaiting for a new connection...\033[0m"
	if [ ! -z "$AUTOCONN" ]; then
		echo -e "\033[33m(autoconnecting to '$AUTOCONN')\033[0m"
	fi

	# does not work for some reason...
	# inotifywait -q "/dev/shm" -e create --exclude='.*\.ctrl' | read dir action file
	rm -f .evs.txt
	inotifywait -q "/dev/shm" -e create --exclude='.*\.ctrl' --outfile .evs.txt
	read dir action file <<< $(cat .evs.txt)

	SHM="/$file"
	HAVE_GREEN="no"

	if [ ! -z "$AUTOCONN" ]; then
		if echo "$SHM" | grep -q "$AUTOCONN" ; then
			echo -e "\033[32mAutoconnecting to $SHM\033[0m"
			HAVE_GREEN="yes"
		else
			echo -e "\033[33mSkipping autoconnect to $SHM\033[0m"
		fi
	else
		while true; do
			read -p "Do you want to connect to '$file' [Y/n]? " answer
			if [ -z "$answer" -o "$answer" == "y" -o "$answer" == "Y" ]; then
				HAVE_GREEN="yes"
				break
			elif [ "$answer" == "n" ]; then
				echo "Very well, skipping..."
				break
			else
				echo "Invalid answer, try again!"
			fi
		done
	fi

	if [ "$HAVE_GREEN" == "yes" ]; then
		echo "Command: " $MON $STREAM:generic:"/$file"
		time $MON "$STREAM:generic:$SHM" ||
			echo -e "\033[31;1m*** Failed running monitor! ***\033[0m"
	fi
done
