#!/bin/sh
MODULE="tracer"
MAIN_DEVICE="tracer"
USER_DEVICE="tracerU"
GROUP="root"
MODE="664"

# invoke insmod 
/sbin/insmod -f ${MODULE} $*

# tell the user about what we are doing
echo "Deleting old tracer nodes: /dev/tracer and /dev/tracerU"

# remove stale nodes
rm -f /dev/${MAIN_DEVICE}
rm -f /dev/${USER_DEVICE}

# look for the major number
MAJOR=`/bin/cat /proc/devices | /usr/bin/awk "\\$2==\"${MODULE}\" {print \\$1}"`

# does the device exist?
if [ ${MAJOR} ]
then
    echo "Found tracer device with major number:" ${MAJOR}
else
    echo "Did not find tracer device ... /dev entries not created ..."
    exit 1
fi

# tell the user about what we are doing
echo "Creating new tracer nodes: /dev/tracer and /dev/tracerU"

# create the character special file
/bin/mknod /dev/${MAIN_DEVICE} c ${MAJOR} 0
/bin/mknod /dev/${USER_DEVICE} c ${MAJOR} 1

# set permissions and ownership
/bin/chgrp ${GROUP} /dev/${MAIN_DEVICE}
/bin/chmod ${MODE}  /dev/${MAIN_DEVICE}
/bin/chgrp ${GROUP} /dev/${USER_DEVICE}
/bin/chmod ${MODE}  /dev/${USER_DEVICE}
