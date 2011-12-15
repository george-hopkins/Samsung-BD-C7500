NDUMP_mtd1_XTRA_OPTS='-l 1048576'  

PREFIX=/dev/
PARTITION_ONE_NAME=$PREFIX$WEAR_TEST_NAME1

echo ">> Test on $PARTITION_ONE_NAME and $PARTITION_TWO_NAME"

if  touch foo 2>/dev/null ; then echo ; else echo; echo ERROR: You must first run chmod oug+rwx on this directory ; echo ; fi

echo
echo ">> uname -r"
uname -r

echo
echo ">> ls -l $IMAGE_NAME"
ls -l $IMAGE_NAME

echo
echo ">> cat /proc/mtd | grep rootfs"
cat /proc/mtd | grep rootfs

#echo
#echo ">> flash_erase $PARTITION_ONE_NAME 0 $WEAR_TEST_BLOCKS_TO_ERASE"
#flash_erase $PARTITION_ONE_NAME 0 $WEAR_TEST_BLOCKS_TO_ERASE
#RETCODE=$?
#echo "Return code: $RETCODE"
#if test "$RETCODE" != "0"; then
#   echo "Cannot erase flash"  >>$FILE_RESULTS 
#   exit 0 
#fi 

echo
echo ">> nandwrite -e -p $PARTITION_ONE_NAME $IMAGE_NAME"
nandwrite -e -p $PARTITION_ONE_NAME $IMAGE_NAME
RETCODE=$?
echo "Return code: $RETCODE"
if test "$RETCODE" != "0"; then
   echo "Cannot nandwrite"  >>$FILE_RESULTS 
   exit 0 
fi 

rm -f mtd*.bin
if [ -f $WEAR_TEST_NAME1.bin ] ; then echo something wrong ; exit 1 ; fi

echo
echo ">> ls -l $ROOT_RESULTS/mtd*.bin"
ls -l $ROOT_RESULTS/mtd*.bin

echo
echo ">> nanddump $NDUMP_mtd1_XTRA_OPTS -f $ROOT_RESULTS/$WEAR_TEST_NAME1.bin $PARTITION_ONE_NAME"
nanddump $NDUMP_mtd1_XTRA_OPTS -f $ROOT_RESULTS/$WEAR_TEST_NAME1.bin $PARTITION_ONE_NAME
RETCODE=$?
echo "Return code: $RETCODE"
if test "$RETCODE" != "0"; then
   echo "Cannot nanddump"  >>$FILE_RESULTS 
   exit 0 
fi 

echo
echo ">> ls -l $ROOT_RESULTS/mtd*.bin"
ls -l $ROOT_RESULTS/mtd*.bin

echo
echo ">> cmp $ROOT_RESULTS/$WEAR_TEST_NAME1.bin $IMAGE_NAME"
cmp $ROOT_RESULTS/$WEAR_TEST_NAME1.bin $IMAGE_NAME
RETCODE=$?
echo "Return code: $RETCODE"
if test "$RETCODE" != "0"; then
   echo "Wear TEST FAILED"  >>$FILE_RESULTS 
   exit 0 
fi 

echo
echo ">> cmp -l $ROOT_RESULTS/$WEAR_TEST_NAME1.bin  $IMAGE_NAME | tail"
cmp -l $ROOT_RESULTS/$WEAR_TEST_NAME1.bin  $IMAGE_NAME | tail
RETCODE=$?
echo "Return code: $RETCODE"
if test "$RETCODE" != "0"; then
   echo "Wear TEST FAILED"  >>$FILE_RESULTS 
   exit 0 
fi 

echo
echo ">> cmp -l $ROOT_RESULTS/$WEAR_TEST_NAME1.bin  $IMAGE_NAME | wc -l"
cmp -l $ROOT_RESULTS/$WEAR_TEST_NAME1.bin  $IMAGE_NAME | wc -l
RETCODE=$?
echo "Return code: $RETCODE"
if test "$RETCODE" != "0"; then
   echo "Wear TEST FAILED"  >>$FILE_RESULTS 
   exit 0 
fi 

echo "Wear TEST SUCCESS";
echo "Wear TEST SUCCESS"  >>$FILE_RESULTS;

echo >> $BAD_BLOCK_WEAR_TESTS_FILE;
echo "Count $NAND_WEAR_TESTS_COUNTER:" >> $BAD_BLOCK_WEAR_TESTS_FILE;
bbtmod $PARTITION_ONE_NAME -d >> $BAD_BLOCK_WEAR_TESTS_FILE;

echo

exit 1
