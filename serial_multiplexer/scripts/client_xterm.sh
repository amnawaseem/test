#!/bin/bash
PORT=$1
CLIENTS=$2
CMD="stty raw -echo && nc localhost $PORT"

for i in $(seq 1 $CLIENTS);
do
#    xterm -xrm "XTerm*selectToClipboard: true" -e "$CMD" &
     gnome-terminal -x bash -c "$CMD"
done
