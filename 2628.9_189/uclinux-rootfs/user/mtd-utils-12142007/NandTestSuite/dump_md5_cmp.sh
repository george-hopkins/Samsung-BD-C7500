#!/bin/bash
#
# This script:
#   1) nanddumps a partition with oob
#   2) md5sum the nanddumped partition
#
#   then:
#       a) nanddumps it again
#       b) md5sum's it
#       c) compares both md5sum
#       d) if error: exit and print message
#       e) if no error: go back to a


PARTITION=$1
LENGTH=$2
BASE_PATH=$3

echo "nanddump -o -l $LENGTH -f $BASE_PATH/dump.bin $PARTITION"
nanddump -o -l $LENGTH -f $BASE_PATH/dump.bin $PARTITION

echo "md5sum $BASE_PATH/dump.bin > $BASE_PATH/md5_base.txt"
md5sum $BASE_PATH/dump.bin > $BASE_PATH/md5_base.txt
mv $BASE_PATH/dump.bin $BASE_PATH/dump_base.bin

while [ 1 ]; do
    echo "nanddump -o -l $LENGTH -f $BASE_PATH/dump.bin $PARTITION"
    nanddump -o -l $LENGTH -f $BASE_PATH/dump.bin $PARTITION
    
    echo "md5sum $BASE_PATH/dump.bin > $BASE_PATH/md5.txt"
    md5sum $BASE_PATH/dump.bin > $BASE_PATH/md5.txt
    
    echo "cmp $BASE_PATH/md5.txt $BASE_PATH/md5_base.txt"
    cmp $BASE_PATH/md5.txt $BASE_PATH/md5_base.txt
    RESULT=$?   #save the result of the cmp straight-away
    echo "Return code for cmp $BASE_PATH/md5.txt $BASE_PATH/md5_base.txt: $RESULT"

    if test "$RESULT" != "0"; then
        echo "Compare TEST FAILED when checking if the errored partition md5sum matches the correct one"
        exit 0
    else
        echo "Compare TEST SUCCESS when checking if the errored partition md5sum matches the correct one"
    fi
    
done

    
    
    
