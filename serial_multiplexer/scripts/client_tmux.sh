#!/bin/bash
PORT=$1
CLIENTS=$2
CMD="stty raw -echo && nc localhost $PORT"

tmux split-window -h "$CMD"

for i in $(seq 2 $CLIENTS);
do
    tmux split-window "$CMD"
done
