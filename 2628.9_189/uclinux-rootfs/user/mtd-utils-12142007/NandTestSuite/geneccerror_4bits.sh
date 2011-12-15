#!/bin/bash
#
# Filename : geneccerror.sh
#
# Author: Martin Fillion
#
# (c)Broadcom 2009
#
# This script:
#
#  Injects ecc correctable errors (4 bits) into each partial page of given number of 
#  bytes in a partition, in the data only.
#
#    For example, ./geneccerror_4bit.sh /dev/mtd10 1048576 will genenerate ecc 
#     correctable errors (4 bits) into each partial page of the first 1048576 bytes
#     of /dev/mmtd10.
#
# ECC_ERR_PARTITION_NAME: A partition name (/dev/mtd10, for example)
#
# ECC_ERR_PARTIAL_PAGE_TEST_COUNTER : How many partial pages where to put errors
#
# DATA_ECC_ERR_OFS      : A data offset in the partition (0x0 to max_partition)
#
# DATA_ECC_ERR_MASK     : A data mask (0x01 creates one ecc error in the byte 
#                         pointed to by $DATA_ECC_ERR_OFS)
#
# OOB_ECC_ERR_OFS       : An oob offset within partial page pointed to by 
#                         $DATA_ECC_ERR_OFS [0 to 15 incl.]
#
# OOB_ECC_ERR_MASK      : A data mask (0x01 creates one ecc error in the byte 
#                         pointed to by $OOB_ECC_ERR_OFS, in the partial page 
#                         pointed to $DATA_ECC_ERR_OFS)
#BDVD_BOOT_AUTOSTART=y BAPP=./app_player BAPP_OUT=/dev/console /etc/init.d/rcS
PAGE_SIZE=512
OOB_SIZE=16

#Parameters:
# $1: Partition (/dev/mtd10, for example...)
# $2: File size 

ECC_ERR_PARTITION_NAME=$1

let ECC_ERR_PARTIAL_PAGE_TEST_COUNTER=$2/131072*256
ECC_ERR_PARTIAL_PAGE_OFS=0

DATA_ECC_ERR_OFS=0x0
DATA_ECC_ERR_OFS_BASE=0x0
DATA_ECC_ERR_MASK=0x55 # 4-bit data error 
OOB_ECC_ERR_OFS=0
OOB_ECC_ERR_MASK=0  # 0-bit oob error 

echo "ECC_ERR_PARTIAL_PAGE_TEST_COUNTER = $ECC_ERR_PARTIAL_PAGE_TEST_COUNTER"

while [ $ECC_ERR_PARTIAL_PAGE_TEST_COUNTER -gt 0 ]
do
    echo "geneccerror -o $DATA_ECC_ERR_OFS -m $DATA_ECC_ERR_MASK -f $OOB_ECC_ERR_OFS -n $OOB_ECC_ERR_MASK $ECC_ERR_PARTITION_NAME";

    #geneccerror command:
    geneccerror -o $DATA_ECC_ERR_OFS -m $DATA_ECC_ERR_MASK -f $OOB_ECC_ERR_OFS -n $OOB_ECC_ERR_MASK $ECC_ERR_PARTITION_NAME ;
    
    #Increment oob offset:
    let OOB_ECC_ERR_OFS++;
    if [ $OOB_ECC_ERR_OFS = $OOB_SIZE ]; then
        OOB_ECC_ERR_OFS=0
    fi    

    #move to next page:
    let DATA_ECC_ERR_OFS_BASE=$DATA_ECC_ERR_OFS_BASE+$PAGE_SIZE;

    #Increment offset:
    let ECC_ERR_PARTIAL_PAGE_OFS++;
    if [ $ECC_ERR_PARTIAL_PAGE_OFS = $PAGE_SIZE ]; then
        ECC_ERR_PARTIAL_PAGE_OFS=0
    fi  
    
    let DATA_ECC_ERR_OFS=$DATA_ECC_ERR_OFS_BASE+$ECC_ERR_PARTIAL_PAGE_OFS;

    let ECC_ERR_PARTIAL_PAGE_TEST_COUNTER--;
done    

