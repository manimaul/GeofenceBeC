#!/bin/sh
### BEGIN INIT INFO
# Provides:          geofencebe
# Required-Start:    $local_fs $network $named $time $syslog
# Required-Stop:     $local_fs $network $named $time $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Description:       geofence back end service
#
# TO INSTALL
# copy to /etc/init.d/geofencebe
# chmod +x geofencebe
# update-rc.d geofencebe defaults
# touch /var/log/geofencebe.log
# service geofencebe start
#
# TO UNINSTALL
# service geofencebe stop
# update-rc.d -f geofencebe remove
#
### END INIT INFO

SCRIPT=/usr/local/bin/GeoFenceBeC
RUNAS=daemon

PIDFILE=/var/run/geofencebe.pid
LOGFILE=/var/log/geofencebe.log

start() {
  if [ -f /var/run/$PIDNAME ] && kill -0 $(cat /var/run/$PIDNAME); then
    echo 'Service already running' >&2
    return 1
  fi
  echo 'Starting service…' >&2
  local CMD="$SCRIPT &> \"$LOGFILE\" & echo \$!"
  su -c "$CMD" $RUNAS > "$PIDFILE"
  echo 'Service started' >&2
}

stop() {
  if [ ! -f "$PIDFILE" ] || ! kill -0 $(cat "$PIDFILE"); then
    echo 'Service not running' >&2
    return 1
  fi
  echo 'Stopping service…' >&2
  kill -15 $(cat "$PIDFILE") && rm -f "$PIDFILE"
  echo 'Service stopped' >&2
}

case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  retart)
    stop
    start
    ;;
  *)
    echo "Usage: $0 {start|stop|restart}"
esac