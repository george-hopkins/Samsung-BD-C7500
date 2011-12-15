#!/bin/sh
mknod /dev/brcm0 c 30 0
mkdir /dev/input
mknod /dev/input/mice c 13 63
mknod /dev/input/keyboard c 10 150
mknod /dev/input/event0 c 13 64
mknod /dev/input/event1 c 13 65
mknod /dev/input/event2 c 13 66
mknod /dev/input/event3 c 13 67
mknod /dev/input/event4 c 13 68
mknod /dev/input/event5 c 13 69
mknod /dev/input/event6 c 13 70
mknod /dev/input/event7 c 13 71
mknod /dev/input/event8 c 13 72
mknod /dev/input/event9 c 13 73
