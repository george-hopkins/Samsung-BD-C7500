#The write disurb test consist in:
#   1- Erase 3 partitions A,B,C (contiguous)
#   2- Get a dump of A and C and keep it
#   3- Write to partition B
#   4- Dump partition A and C
#   5- Compare against original erased partition, they should still be erased. If not, a write
#      disturbance occurs.
#   6- If no error, Go back to step 3

#NAND_READWRITE_TESTS_COUNTER=10000
#READWRITE_TEST_NAME1=mtd11
#READWRITE_TEST_NAME2=mtd12
#READWRITE_TEST_NAME3=mtd13
#FILE_RESULTS=test.txt
#BAD_BLOCK_WRITE_DISTURB_TESTS_FILE=bbwd.txt

IMAGE_NAME=393Kilo.img
PREFIX=/dev/
PARTITION_ONE_NAME=$PREFIX$READWRITE_TEST_NAME1
PARTITION_TWO_NAME=$PREFIX$READWRITE_TEST_NAME2
PARTITION_THREE_NAME=$PREFIX$READWRITE_TEST_NAME3
ERASE_NAME=_erased.bin
DISTURB_NAME=_disturbed.bin
ERASE1=$READWRITE_TEST_NAME1$ERASE_NAME
ERASE2=$READWRITE_TEST_NAME2$ERASE_NAME
ERASE3=$READWRITE_TEST_NAME3$ERASE_NAME
DISTURB1=$READWRITE_TEST_NAME1$DISTURB_NAME
DISTURB2=$READWRITE_TEST_NAME2$DISTURB_NAME
DISTURB3=$READWRITE_TEST_NAME3$DISTURB_NAME

if  touch foo 2>/dev/null ; then echo ; else echo; echo ERROR: You must first run chmod oug+rwx on this directory ; echo ; fi

RESULT=0

echo
echo ">> uname -r"
uname -r

echo
echo ">> ls -l $IMAGE_NAME"
ls -l $IMAGE_NAME

echo
echo ">> cat /proc/mtd | grep rootfs"
cat /proc/mtd | grep rootfs

#Erase partitions A,B,C

echo
echo ">> flash_eraseall $PARTITION_ONE_NAME"
flash_eraseall $PARTITION_ONE_NAME

echo
echo ">> flash_eraseall $PARTITION_TWO_NAME"
flash_eraseall $PARTITION_TWO_NAME

echo
echo ">> flash_eraseall $PARTITION_THREE_NAME"
flash_eraseall $PARTITION_THREE_NAME

#Get the content of partitions A and C

echo
echo ">> nanddump -o -b -f $ROOT_RESULTS/$ERASE1 $PARTITION_ONE_NAME"
nanddump -o -b -f $ROOT_RESULTS/$ERASE1 $PARTITION_ONE_NAME

echo 
echo ">> nanddump -o -b -f $ROOT_RESULTS/$ERASE3 $PARTITION_THREE_NAME"
nanddump -o -b -f $ROOT_RESULTS/$ERASE3 $PARTITION_THREE_NAME

while [ $NAND_READWRITE_TESTS_COUNTER -gt 0 ]
do

    echo 'executing Write Disturb Test - COUNTER:' $NAND_READWRITE_TESTS_COUNTER >> $FILE_RESULTS;
    echo 'executing Write Disturb Test - COUNTER:' $NAND_READWRITE_TESTS_COUNTER;

    let NAND_READWRITE_TESTS_COUNTER--
    nandinfo -f $BAD_BLOCK_WRITE_DISTURB_TESTS_FILE >null
    
    #Write data in partition B

    echo
    echo ">> nandwrite -p $PARTITION_TWO_NAME $IMAGE_NAME"
    nandwrite -p $PARTITION_TWO_NAME $IMAGE_NAME

    # check that we still have 0xFF in partitions A and C
    
    echo
    echo ">> nanddump -o -b -f $ROOT_RESULTS/$DISTURB1 $PARTITION_ONE_NAME"
    nanddump -o -b -f $ROOT_RESULTS/$DISTURB1 $PARTITION_ONE_NAME

    echo 
    echo ">> nanddump  -o -b -f $ROOT_RESULTS/$DISTURB3 $PARTITION_THREE_NAME"
    nanddump -o -b -f $ROOT_RESULTS/$DISTURB3 $PARTITION_THREE_NAME

    echo
    echo ">> ls -l $ROOT_RESULTS/mtd*.bin"
    ls -l $ROOT_RESULTS/mtd*.bin

    echo
    echo ">> cmp $ROOT_RESULTS/$DISTURB1  $ROOT_RESULTS/$ERASE1"
    cmp $ROOT_RESULTS/$DISTURB1  $ROOT_RESULTS/$ERASE1
    if test "$?" != "0"; then
        echo "Write Disturb TEST FAILED"  >>$FILE_RESULTS 
        nandinfo  >>$FILE_RESULTS
    fi 
    
    echo
    echo ">> cmp $ROOT_RESULTS/$DISTURB3  $ROOT_RESULTS/$ERASE3"
    cmp $ROOT_RESULTS/$DISTURB3  $ROOT_RESULTS/$ERASE3
    if test "$?" != "0"; then
        echo "Write Disturb TEST FAILED"  >>$FILE_RESULTS 
        nandinfo  >>$FILE_RESULTS
    fi    
    
    echo "Write Disturb TEST SUCCESS"  >>$FILE_RESULTS;    
    
    echo
    echo ">> flash_eraseall $PARTITION_TWO_NAME"
    flash_eraseall $PARTITION_TWO_NAME

    echo    
done

