#!/bin/bash
# This script ensures that the server is always running. 
# If the sever stops working for any reason, this script
# will restart the server. It works by searching if the gamerserver process is running using pgrep, and restarts it if the process
# is not found.

check_process() {
  echo "$ts: checking $1"
  [ "$1" = "" ]  && return 0
  [ `pgrep -n $1` ] && return 1 || return 0
}

while [ 1 ]; do 
  # timestamp
  ts=`date +%T`

  echo "$ts: begin checking..."
  check_process "server"
  [ $? -eq 0 ] && echo "$ts: not running, restarting..."	 && `./../../tcp_game/server/server`
  sleep 5
done
