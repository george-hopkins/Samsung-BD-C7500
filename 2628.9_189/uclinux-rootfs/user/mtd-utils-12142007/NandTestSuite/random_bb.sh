#!/bin/bash
#
# Filename : random_bb.sh
#
# Author: Martin Fillion
#
# (c)Broadcom 2009
#
# This script:
#  1 - Generates a random bad block, 
#  2 - Verifies if it is correctly marked in the bbt (WEAR)
#  3 - Erases the bbt
#  4 - Recreates the bbt
#  5 - Verifies if it is still marked in the bbt (FACTORY)
#  6 - Unmarks the block
#  5 - Verifies if it is not marked as bad in the bbt anymore
#

echo "RANDOM_BB_PARTITION:" $RANDOM_BB_PARTITION
echo "FILE_RESULTS:" $FILE_RESULTS
echo "Marking block as bad and getting random_address"

echo "Generating a random bad block in an erased block" >>$FILE_RESULTS 
RANDOM_ADDRESS=`bbtmod -m $RANDOM_BB_PARTITION | grep 'random_blockstart = ' | cut -c 21-28`
echo "Random address: " $RANDOM_ADDRESS
echo "Random address: " $RANDOM_ADDRESS >>$FILE_RESULTS

echo "Check that it is correctly marked as BAD, DUE TO WEAR" >>$FILE_RESULTS
TXT="Bad block DUE TO WEAR at ${RANDOM_ADDRESS}"
bbtmod -d $RANDOM_BB_PARTITION | grep "$TXT"
RESULT=$?   #save the result of the grep straight-away
echo "Return code for grep '$TXT': $RESULT" >>$FILE_RESULTS
if test "$RESULT" != "0"; then
    echo "Random bad block TEST FAILED when checking that the random block is marked as bad (WEAR)"  >>$FILE_RESULTS 
    exit 0 
else
    echo "Random bad block TEST SUCCESS when checking that the random block is marked as bad (WEAR)"  >>$FILE_RESULTS
fi

echo "Erasing BBT" >>$FILE_RESULTS
bbtmod -e

echo "Rescanning BBT" >>$FILE_RESULTS
bbtmod -s

echo "Checking that it is still marked as BAD, but this time FACTORY MARKED" >>$FILE_RESULTS
TXT="Bad block FACTORY MARKED at ${RANDOM_ADDRESS}"
bbtmod -d $RANDOM_BB_PARTITION | grep "$TXT"
RESULT=$?   #save the result of the grep straight-away
echo "Return code for grep '$TXT': $RESULT" >>$FILE_RESULTS
if test "$RESULT" != "0"; then
   echo "Random bad block TEST FAILED when checking that the random block is marked as bad (FACTORY)"  >>$FILE_RESULTS 
   exit 0
else
    echo "Random bad block TEST SUCCESS when checking that the random block is marked as bad (FACTORY)"  >>$FILE_RESULTS
fi

echo "Setting the block back to GOOD" >>$FILE_RESULTS
bbtmod -o 0x${RANDOM_ADDRESS} -t G $RANDOM_BB_PARTITION

echo "Checking that it is not marked bad anymore" >>$FILE_RESULTS
bbtmod -d $RANDOM_BB_PARTITION | grep "$TXT"
RESULT=$?   #save the result of the grep straight-away
echo "Return code for grep '$TXT': $RESULT" >>$FILE_RESULTS
if test "$RESULT" != "0"; then
    echo "Random bad block TEST SUCCESS" >>$FILE_RESULTS    
else
    echo "Random bad block TEST FAILED : could not mark it as GOOD" >>$FILE_RESULTS    
fi
