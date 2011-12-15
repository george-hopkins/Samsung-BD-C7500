#!/bin/bash
#
#   This script:
#       Takes 3 parameters:
#           1. Number of iterations
#           2. mtdblock1 device (without the /dev)
#           3. mountpoint1 for yaffs2 (withouht the /dev Ex.: pstor, hd)
#           4. mtdblock2 device (without the /dev)
#           5. mountpoint2 for yaffs2 (withouht the /dev Ex.: pstor, hd)
#           6. filename to untar
#   Description:
#       1. Mount yaffs2 partition
#       2. Untar root-based rootfsB file into mountpoint
#       3. List the files located into the untarred partition
#       4. Appends all the files located in the partition into a big "glom"
#       5. md5sum glom.1
#       6. erase all files in yaffs2 partition
#       7. umount yaffs2 partition
#       8. Does the same as 2,3,4,5,6,7 into mountpoint2
#       9. cmp both md5sum.  
#       10. If they don't match: output error message and exit
#       11. If they match: go back to 1, until the number of iterations ends

COUNTER=$1
while [ $COUNTER -gt 0 ]
do
    echo "COUNTER: " $COUNTER

    date

    echo "mount -t yaffs2 /dev/$2 /mnt/$3"
    mount -t yaffs2 /dev/$2 /mnt/$3

    cd /mnt/$3
    echo "pwd is: "    
    pwd

    echo "tar -xzf $6 -C /mnt/$3"
    tar -xzf $6 -C /mnt/$3

    echo "find ./ -type f > /list"
    find ./ -type f > /list

    echo "cat `cat /list` > /glom.1"
    cat `cat /list` > /glom.1

    echo "md5sum /glom.1 > /rs.1"
    md5sum /glom.1 > /rs.1

    echo "rm -rf ./"	
    rm -rf ./

    echo "cd /" 
    cd /
    
    echo "umount /mnt/$3"
    umount /mnt/$3
    
    mv /glom.1 /glom.a
#-------------------------------------------------
    echo "mount -t yaffs2 /dev/$4 /mnt/$5"
    mount -t yaffs2 /dev/$4 /mnt/$5

    cd /mnt/$5
    echo "pwd is: "    
    pwd

    echo "tar -xzf $6 -C /mnt/$5"
    tar -xzf $6 -C /mnt/$5

    echo "cat `cat /list` > /glom.1"
    cat `cat /list` > /glom.1

    echo "md5sum /glom.1 > /rs.2"
    md5sum /glom.1 > /rs.2

    echo "rm -rf ./"	
    rm -rf ./

    echo "cd /" 
    cd /
    echo "umount /mnt/$5"
    umount /mnt/$5

    mv /glom.1 /glom.b
#-------------------------------------------------        
    echo "cmp /rs.1 /rs.2"
    cmp /rs.1 /rs.2
    RESULT=$?   #save the result of the cmp straight-away
    echo "Return code for cmp /rs.1 /rs.2: $RESULT"

    if test "$RESULT" != "0"; then
        echo "Iteration $COUNTER: md5sum Compare TEST FAILED"
        echo "Iteration $COUNTER: md5sum Compare TEST FAILED" >> cmp_res.txt
        exit 0
    else
        echo "Iteration $COUNTER: md5sum Compare TEST SUCCESS" 
        echo "Iteration $COUNTER: md5sum Compare TEST SUCCESS" >> cmp_res.txt
    fi  
    
    let COUNTER--      
done
