#!/bin/bash
#
# Filename : random_bb.sh
#
# Author: Jean Roberge/Martin Fillion
#
# (c)Broadcom 2008-2009
#
# Menu for NAND Acceptance Testing.

#This suffix helps to avoid overwritting previous data result
SUFFIX=`date +%y_%m_%d_%H_%M_%S`

#This variable is exported so external script can use it
export IMAGE_NAME=test_1M.bin
IMAGE_NAME_BACKUP=test_1M.bin

#This variable is exported so external script can use it to store data results
#General Results
export FILE_RESULTS_1000=1000_iterations_$SUFFIX.txt

export FILE_RESULTS=nand_result_$SUFFIX.txt
FILE_RESULTS_BACKUP=nand_result_$SUFFIX.txt

#Wear tests results file
export BAD_BLOCK_WEAR_TESTS_FILE=bbwtest_$SUFFIX.txt          
BAD_BLOCK_WEAR_TESTS_FILE_BACKUP=bbw_test_$SUFFIX.txt

#Read/Write disturb results file
export BAD_BLOCK_READ_DISTURB_TESTS_FILE=bbrd_test_$SUFFIX.txt 
BAD_BLOCK_READ_DISTURB_TESTS_FILE_BACKUP=bbrd_test_$SUFFIX.txt
export BAD_BLOCK_WRITE_DISTURB_TESTS_FILE=bbwd_test_$SUFFIX.txt   
BAD_BLOCK_WRITE_DISTURB_TESTS_FILE_BACKUP=bbwd_test_$SUFFIX.txt     

#Read/Write disturb variables
export NAND_READWRITE_TESTS_COUNTER=100000
NAND_READWRITE_TESTS_COUNTER_BACKUP=100000

export READWRITE_TEST_NAME1=mtd11
export READWRITE_TEST_NAME2=mtd12
export READWRITE_TEST_NAME3=mtd13
READWRITE_TEST_NAME1_BACKUP=mtd11
READWRITE_TEST_NAME2_BACKUP=mtd12
READWRITE_TEST_NAME3_BACKUP=mtd13

#Wear tests variables
export WEAR_TEST_NAME1=mtd10
WEAR_TEST_NAME1_BACKUP=mtd10

export NAND_WEAR_TESTS_COUNTER=500000
NAND_WEAR_TESTS_COUNTER_BACKUP=500000

#Throughput Test Variables

PARTITION_NAME=/dev/mtd21
THROUGHPUT_MTD_PARTITION_NAME=/dev/mtd21
THROUGHPUT_MTDBLOCK_PARTITION_NAME=/dev/mtdblock21

#Random Bad Block Test Variables
#This variable is exported so external script can use it to store data results
export RANDOM_BB_PARTITION=/dev/mtd10
RANDOM_BB_PARTITION_BACKUP=/dev/mtd10

#BBT auto re-scan test
RANDOM_BB_PARTITION_A=/dev/mtd16
RANDOM_BB_PARTITION_BACKUP_A=/dev/mtd16
RANDOM_ADDRESS_A=00000000

RANDOM_BB_PARTITION_B=/dev/mtd17
RANDOM_BB_PARTITION_BACKUP_B=/dev/mtd16
RANDOM_ADDRESS_B=00000000

RANDOM_BB_PARTITION_C=/dev/mtd18
RANDOM_BB_PARTITION_BACKUP_C=/dev/mtd18
RANDOM_ADDRESS_C=00000000

# This is the default ecc error counts (for bch4).
ECC_COR_ERROR_CNT=4
ECC_UNCOR_ERROR_CNT=5

export ROOT_RESULTS=.

#Start by changing the test file name
echo "Default Results/Statistics File Name: "$FILE_RESULTS;    
echo "Enter File Name: " ; read FILE_RESULTS_BACKUP;

if test "$FILE_RESULTS_BACKUP" != ""; then
    FILE_RESULTS=$FILE_RESULTS_BACKUP
fi 
echo "Using Name: "$FILE_RESULTS
  
date > $FILE_RESULTS
echo " " >> $FILE_RESULTS
uname -a >> $FILE_RESULTS
echo " " >> $FILE_RESULTS
ifconfig >> $FILE_RESULTS

Other_Parameters_Menu()
{
    while [ 1 ]
    do
         echo "---------------- Other Parameters Menu --------------------------------------"
         echo " Wear Tests File for Graphic Application: "$BAD_BLOCK_WEAR_TESTS_FILE          
         echo " Read Disturb File for Graphic Application: "$BAD_BLOCK_READ_DISTURB_TESTS_FILE 
         echo " Write Disturb File for Graphic Application: "$BAD_BLOCK_WRITE_DISTURB_TESTS_FILE         
         echo "-----------------------------------------------------------------------------"  
         echo "[0] Change File Name for Wear Tests"         
         echo "[1] Change File Name for Read Disturb Tests"    
         echo "[2] Change File Name for Write Disturb Tests"
         echo "[3] Return to Main Menu"
         echo "=============================================================="
         echo -n "Enter your menu choice [0 to 3]: "
         read yourch
         case $yourch in
      0) echo "Enter File Name for Wear Tests: ";  
              read BAD_BLOCK_WEAR_TESTS_FILE_BACKUP;
              if test "$BAD_BLOCK_WEAR_TESTS_FILE_BACKUP" != ""; then
                 BAD_BLOCK_WEAR_TESTS_FILE=$BAD_BLOCK_WEAR_TESTS_FILE_BACKUP
              fi;;                             
      1) echo "Enter File Name for Read Disturb Tests: ";                
              read BAD_BLOCK_READ_DISTURB_TESTS_FILE_BACKUP;
              if test "$BAD_BLOCK_READ_DISTURB_TESTS_FILE_BACKUP" != ""; then
                 BAD_BLOCK_READ_DISTURB_TESTS_FILE=$BAD_BLOCK_READ_DISTURB_TESTS_FILE_BACKUP
              fi;;  
      2) echo "Enter File Name for Write Disturb Tests: ";                
              read BAD_BLOCK_WRITE_DISTURB_TESTS_FILE_BACKUP;
              if test "$BAD_BLOCK_WRITE_DISTURB_TESTS_FILE_BACKUP" != ""; then
                 BAD_BLOCK_WRITE_DISTURB_TESTS_FILE=$BAD_BLOCK_WRITE_DISTURB_TESTS_FILE_BACKUP
              fi;;                                     
      3) return;;
      *) return;;
     esac
    done
    return    
}

ReadWrite_Disturb_Tests_Menu()
{
    while [ 1 ]
    do
         echo "---------------- Read/Write Disturb Tests Menu ------------------------------"
         echo " Number of Iterations for Read/Write Disturb testing: "$NAND_READWRITE_TESTS_COUNTER          
         echo " Read/Write Disturb testing will be done on "$READWRITE_TEST_NAME1", "$READWRITE_TEST_NAME2" and "$READWRITE_TEST_NAME3
         echo "-----------------------------------------------------------------------------"  
         echo "[0] How many times the test will be done"         
         echo "[1] Change First Partition Name"    
         echo "[2] Change Second Partition Name"
         echo "[3] Change Third Partition Name"                                 
         echo "[4] Start Read Disturb Test"
         echo "[5] Start Write Disturb Test"  
         echo "[6] Return to Main Menu"
         echo "=============================================================="
         echo -n "Enter your menu choice [0 to 6]: "
         read yourch
         case $yourch in
      0) echo "Enter Number of Iterations Wanted for the Read/Write Disturb Tests: "; 
              read NAND_READWRITE_TESTS_COUNTER;;         
      1) echo "Enter First Partition Name: ";  
              read READWRITE_TEST_NAME1_BACKUP;
              if test "$READWRITE_TEST_NAME1_BACKUP" != ""; then
                 READWRITE_TEST_NAME1=$READWRITE_TEST_NAME1_BACKUP
              fi;;                             
      2) echo "Enter Second Partition Name: ";                
              read READWRITE_TEST_NAME2_BACKUP;
              if test "$READWRITE_TEST_NAME2_BACKUP" != ""; then
                 READWRITE_TEST_NAME2=$READWRITE_TEST_NAME2_BACKUP
              fi;;  
      3) echo "Enter Second Partition Name: ";                
              read READWRITE_TEST_NAME3_BACKUP;
              if test "$READWRITE_TEST_NAME3_BACKUP" != ""; then
                 READWRITE_TEST_NAME3=$READWRITE_TEST_NAME3_BACKUP
              fi;;                                       
      4) echo "------------------------------------------" >>$FILE_RESULTS;
         echo "Read Disturb Test" >>$FILE_RESULTS;
         echo "------------------------------------------" >>$FILE_RESULTS;    
         nandstat >>$FILE_RESULTS; 
         nandinfo >>$FILE_RESULTS;  
         Read_Disturb_Test;
         nandstat >>$FILE_RESULTS; 
         nandinfo >>$FILE_RESULTS;          
         echo "Press Any Key To Continue..." ; read;;
      5) echo "------------------------------------" >>$FILE_RESULTS; 
         echo "Write Disturb Test" >>$FILE_RESULTS;
         echo "------------------------------------" >>$FILE_RESULTS;       
         nandstat >>$FILE_RESULTS; 
         nandinfo >>$FILE_RESULTS;          
         Write_Disturb_Test;
         nandstat >>$FILE_RESULTS; 
         nandinfo >>$FILE_RESULTS;          
         echo "Press Any Key To Continue..." ; read;;                   
      6) return;;
      *) return;;
     esac
    done
    return    
}

Throughput_Tests_Menu()
{
    while [ 1 ]
    do
         echo "---------------- Throughput Tests Menu --------------------------------------"
         echo " File System Throughput Tests will be done on MTD partition:"$THROUGHPUT_MTD_PARTITION_NAME 
         echo " RAW Throughput Tests will be done on MTDBLOCK partition:"$THROUGHPUT_MTDBLOCK_PARTITION_NAME         
         echo "-----------------------------------------------------------------------------"  
         echo "[0] Change File System Partition Name for Throughput Testing"    
         echo "[1] Change RAW Partition Name for Throughput Testing"                        
         echo "[2] Start Throughput Testing with YAFFS2"
         echo "[3] Start Throughput Testing in RAW Mode"  
         echo "[4] Return to Main Menu"
         echo "=============================================================="
         echo -n "Enter your menu choice [0 to 4]: "
         read yourch
         case $yourch in
      0) echo "Enter new FS Partition Name: " ; read THROUGHPUT_MTD_PARTITION_NAME;;          
      1) echo "Enter new RAW Partition Name: " ; read THROUGHPUT_MTDBLOCK_PARTITION_NAME;;                
      2) echo "------------------------------------------" >>$FILE_RESULTS;
         echo "Throughput Tests with File System (yaffs2)" >>$FILE_RESULTS;
         echo "------------------------------------------" >>$FILE_RESULTS;      
         Throughput_Tests_with_File_System;
         echo "Press Any Key To Continue..." ; read;;
      3) echo "------------------------------------" >>$FILE_RESULTS; 
         echo "Throughput Tests with NO File System" >>$FILE_RESULTS;
         echo "------------------------------------" >>$FILE_RESULTS;       
         Throughput_Tests_with_NO_File_System;
         echo "Press Any Key To Continue..." ; read;;                   
      4) return;;
      *) return;;
     esac
    done
    return    
}

Generate_ECC_Error_Menu()
{
    while [ 1 ]
    do
        echo "---------------- Generate ECC Error Menu -----------------------------"
        echo " Generation of ECC Correctable Errors will be done on $PARTITION_NAME "
        echo " Number of bits to put in error to generate ECC Correctable (THRESHOLD"
        echo " is set to 3 on bch4): $ECC_COR_ERROR_CNT                             "
        echo " Number of bits to put in error to generate ECC Uncorrectable         "
        echo " (THRESHOLD is set to : $ECC_UNCOR_ERROR_CNT)                         "
        echo "----------------------------------------------------------------------"  
        echo "[0] Change Partition Name for ECC Correctable Error Partition         "
        echo "[1] Change Number of bits to to generate ECC Correctable              "
        echo "[2] Change Number of bits to generate ECC Uncorrectable               "
        echo "[3] Generate ECC Correctable                                          "
        echo "[4] Generate ECC Uncorrectable Error                                  "
        echo "[5] Return to Main Menu                                               "
        echo "======================================================================"
        echo -n "Enter your menu choice [0 to 5]: "
        read yourch
        case $yourch in
      0) echo "Enter new Partition Name: " ; read PARTITION_NAME;; 
      1) echo "Enter new number of bits to put in correctable error: " ; read ECC_COR_ERROR_CNT ;;         
      2) echo "Enter new number of bits to put in uncorrectable error: " ; read ECC_UNCOR_ERROR_CNT ;;         
      3) echo "-----------------------------------------------" >>$FILE_RESULTS;
         echo "Generate ECC Correctable Error                 " >>$FILE_RESULTS;
         echo "-----------------------------------------------" >>$FILE_RESULTS; 
         nandstat >>$FILE_RESULTS;              
         Generate_ECC_Correctable_Error;
         nandstat >>$FILE_RESULTS;         
         echo "Press Any Key To Continue..." ; read ;;
      4) echo "----------------------------------------------------" >>$FILE_RESULTS;
         echo "Generate ECC Uncorrectable Error                    " >>$FILE_RESULTS;
         echo "----------------------------------------------------" >>$FILE_RESULTS; 
         nandstat >>$FILE_RESULTS;              
         Generate_ECC_Uncorrectable_Error;       
         nandstat >>$FILE_RESULTS;         
         echo "Press Any Key To Continue..." ; read ;;  
      5) return;;
      *) return;;
     esac
    done
    return    
}

Bad_Block_Table_Modifications_Menu()
{
    while [ 1 ]
    do
        echo "---------------- Bad Block Table Modification Menu ---------------------"
        echo "[0] Show Bad Block Table"
        echo "[1] Change a block status for an MTD partitions"    
        echo "[2] Change a block status at an absolute address"    
        echo "[3] Erase bad Block Table"        
        echo "[4] Return to Main Menu"
        echo "========================================================================="
        echo -n "Enter your menu choice [0 to 4]: "
        read yourch
        case $yourch in
           0) echo "Bad Block Table";
              bbtmod -d; 
              echo "Press Any Key To Continue..." ; read;;
           1) echo "Enter MTD Partition Name";
              read partitionTableName;
              echo "Enter Relative offset to where you want the status change";
              read relativeOffset;
              echo "Enter new Block status";
              echo "(G:Good Block W:Bad due to Wear R:Reserved F:Factory Bad Block Marked";
              read blockStatus;
              bbtmod $partitionTableName -o $relativeOffset -t $blockStatus                             
              echo "Press Any Key To Continue..." ; read;;
           2) echo "Enter Absolute Offset";
              read absoluteOffset;
              echo "Enter new Block status";
              echo "(G:Good Block W:Bad due to Wear R:Reserved F:Factory Bad Block Marked";
              read blockStatus; 
 #            /dev/mtd0 is not used here           
              bbtmod /dev/mtd0 -a $absoluteOffset -t $blockStatus   
              echo "Press Any Key To Continue..." ; read;;  
           3) bbtmod -e; 
              echo "Press Any Key To Continue..." ; read;;                                      
           4) return;;
           *) return;;
        esac
    done
    return    
}

Read_Disturb_Test()
{
         NAND_READWRITE_TESTS_COUNTER_BACKUP=$NAND_READWRITE_TESTS_COUNTER;
         ./read_disturb_test.sh >> detailed$FILE_RESULTS; 
         NAND_READWRITE_TESTS_COUNTER=$NAND_READWRITE_TESTS_COUNTER_BACKUP;         
         return
}

Write_Disturb_Test()
{
         NAND_READWRITE_TESTS_COUNTER_BACKUP=$NAND_READWRITE_TESTS_COUNTER;
         ./write_disturb_test.sh >> detailed$FILE_RESULTS;
         NAND_READWRITE_TESTS_COUNTER_BACKUP=$NAND_READWRITE_TESTS_COUNTER;         
         return
}

Throughput_Tests_with_NO_File_System()
{
         echo "flash_eraseall on $THROUGHPUT_MTD_PARTITION_NAME";
         flash_eraseall $THROUGHPUT_MTD_PARTITION_NAME ; 
         echo "ft_mips $THROUGHPUT_MTD_PARTITION_NAME -raw";
         ft_mips $THROUGHPUT_MTD_PARTITION_NAME -raw >>$FILE_RESULTS; 
         return
}

Throughput_Tests_with_File_System()
{
         echo "flash_eraseall on $THROUGHPUT_MTD_PARTITION_NAME";
         flash_eraseall $THROUGHPUT_MTD_PARTITION_NAME ; 
         mount -t yaffs2 $THROUGHPUT_MTDBLOCK_PARTITION_NAME /mnt/hd ; 
         ft_mips $THROUGHPUT_MTDBLOCK_PARTITION_NAME -hd >>$FILE_RESULTS; 
         umount /mnt/hd ; 
         return
}

Generate_ECC_Correctable_Error()
{
         echo "Generating $ECC_COR_ERROR_CNT errors in $PARTITION_NAME, using automated test" 
         echo "Generating $ECC_COR_ERROR_CNT errors in $PARTITION_NAME, using automated test" >>$FILE_RESULTS; 
         geneccerror -t -b $ECC_COR_ERROR_CNT $PARTITION_NAME >>$FILE_RESULTS; 
         return
}

Generate_ECC_Uncorrectable_Error()
{
         echo "Generating $ECC_UNCOR_ERROR_CNT errors in $PARTITION_NAME, using automated test" 
         echo "Generating $ECC_UNCOR_ERROR_CNT errors in $PARTITION_NAME, using automated test" >>$FILE_RESULTS; 
         geneccerror -t -b $ECC_UNCOR_ERROR_CNT $PARTITION_NAME >>$FILE_RESULTS; 
         return
}


Nand_Wear_Tests()
{
    uname -a >> $FILE_RESULTS_1000
    echo " " >> $FILE_RESULTS_1000
    ifconfig >> $FILE_RESULTS_1000

              NAND_WEAR_TESTS_COUNTER_BACKUP=$NAND_WEAR_TESTS_COUNTER;
              while [ $NAND_WEAR_TESTS_COUNTER -gt 0 ]
                do
                date
                echo 'executing nandtest.sh COUNTER:' $NAND_WEAR_TESTS_COUNTER >> $FILE_RESULTS;
                echo 'executing nandtest.sh COUNTER:' $NAND_WEAR_TESTS_COUNTER;
                ZERO=0
                let "REMAINDER = $NAND_WEAR_TESTS_COUNTER % 1000"
                if [ $REMAINDER -eq 0 ]; then
                    echo "Iteration# $NAND_WEAR_TESTS_COUNTER"
                    echo "Iteration# $NAND_WEAR_TESTS_COUNTER" >> $FILE_RESULTS_1000;
                    echo "date"
                    date >> $FILE_RESULTS_1000;
                    echo "bbtmod -d /dev/$WEAR_TEST_NAME1"
                    bbtmod -d /dev/$WEAR_TEST_NAME1 >>$FILE_RESULTS_1000;
                    
                    # let "REMAINDER = $NAND_WEAR_TESTS_COUNTER % 10000"
                    # if [ $REMAINDER -eq 0 ]; then
                    #    echo "bbtmod -e" >> $FILE_RESULTS_1000
                    #    bbtmod -e;
                    #    echo "bbtmod -s" >> $FILE_RESULTS_1000
                    #    bbtmod -s;
                    # fi
                fi
                ./WearTest.sh >> detailed$FILE_RESULTS;
                RESULT=$?   #save the result of the WearTest straight-away 
                if test "$RESULT" != "1"; then
                        #Run it again
                        ./WearTest.sh >> detailed$FILE_RESULTS;
                        RESULT=$?   #save the result of the WearTest straight-away 
                        if test "$RESULT" != "1"; then
                            echo "WearTest Failure"
                            #exit 1  
                        else
                            echo "WearTest Success on second pass"
                        fi
                        
                else
                        echo "WearTest Success"
                fi 
                
                let NAND_WEAR_TESTS_COUNTER--  
                
              done
              NAND_WEAR_TESTS_COUNTER=$NAND_WEAR_TESTS_COUNTER_BACKUP;              
              return
}

Nand_Wear_Tests_Menu()
{
    while [ 1 ]
    do
        echo "----------------------- Nand Wear Tests Menu -------------------------"
        echo " How many times the test will be done: "$NAND_WEAR_TESTS_COUNTER      "
        echo " Wear testing will be done on "$WEAR_TEST_NAME1                       "
        echo " The file "$IMAGE_NAME" will be copied                                "         
        echo "----------------------------------------------------------------------"          
        echo "[0] How many times the test will be done"
        echo "[1] Change File Name to be copied"         
        echo "[2] Change Name For Partition"
        echo "[3] Start Wear Test"    
        echo "[4] Return to Main Menu"
        echo "========================================================================="
        echo -n "Enter your menu choice [0 to 5]: "
        read yourch
        case $yourch in
           0) echo "Enter Number of Iterations Wanted for the Tests: "; 
              read NAND_WEAR_TESTS_COUNTER;;
           1) echo "Enter file name: ";
              read IMAGE_NAME_BACKUP;
              if test "$IMAGE_NAME_BACKUP" != ""; then
                 IMAGE_NAME=$IMAGE_NAME_BACKUP
              fi;;                                                                         
           2) echo "Partition Name: "; 
              read WEAR_TEST_NAME1_BACKUP;
              if test "$WEAR_TEST_NAME1_BACKUP" != ""; then
                 WEAR_TEST_NAME1=$WEAR_TEST_NAME1_BACKUP
              fi;;               
           3) Nand_Wear_Tests;;
           4) return;;
           *) return;;
        esac
    done
    return    
}

Random_BB_Test()
{
    ./random_bb.sh;
}

Random_BB_Test_Menu()
{
    while [ 1 ]
    do
        echo "--------------------- Random Bad Block TEst Menu ---------------------"
        echo " Random Bad Block testing will be done on "$RANDOM_BB_PARTITION
        echo "----------------------------------------------------------------------"          
        echo "[0] Change Partition Name. Make sure it has been erased prior to this test."
        echo "[1] Start Random Bad Block Test"    
        echo "[2] Return to Main Menu"
        echo "======================================================================"
        echo -n "Enter your menu choice [0 to 2]: "
        read yourch
        case $yourch in
           0) echo "Partition name: "; 
              read RANDOM_BB_PARTITION_BACKUP;
              if test "$RANDOM_BB_PARTITION_BACKUP" != ""; then
                 RANDOM_BB_PARTITION=$RANDOM_BB_PARTITION_BACKUP
              fi;;
           1) Random_BB_Test;;
           2) return;;
           *) return;;
        esac
    done
    return    

}

nw_BBT_Auto_Re_Scan_Test()
{
    bbtmod -d $RANDOM_BB_PARTITION_A;
    bbtmod -d $RANDOM_BB_PARTITION_B;

#    bbtmod -r $RANDOM_BB_PARTITION_A;
#    bbtmod -r $RANDOM_BB_PARTITION_B;

    RANDOM_ADDRESS_A=`bbtmod -r $RANDOM_BB_PARTITION_A | grep "random_blockstart = " | cut -c 21-28`;
    RANDOM_ADDRESS_B=`bbtmod -r $RANDOM_BB_PARTITION_B | grep "random_blockstart = " | cut -c 21-28`;
                
}
BBT_Auto_Re_Scan_Test()
{
    echo "Marking 3 blocks as bad and getting random_addresses"
   
    echo "-------------------------------" >>$FILE_RESULTS
    echo "Initial bbt:                   " >>$FILE_RESULTS
    bbtmod -d $RANDOM_BB_PARTITION_A >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_B >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_C >> $FILE_RESULTS;    

    echo "-------------------------------" >>$FILE_RESULTS;

    echo "Generating random bad block in" $RANDOM_BB_PARTITION_A >>$FILE_RESULTS;
    
    RANDOM_ADDRESS_A=`bbtmod -r $RANDOM_BB_PARTITION_A | grep "random_blockstart = " | cut -c 21-28`;
    echo "Random address A: " $RANDOM_ADDRESS_A >>$FILE_RESULTS;
    
    echo "Generating random bad block in" $RANDOM_BB_PARTITION_B >>$FILE_RESULTS;
    RANDOM_ADDRESS_B=`bbtmod -r $RANDOM_BB_PARTITION_B | grep "random_blockstart = " | cut -c 21-28`;
    echo "Random address B: " $RANDOM_ADDRESS_B >>$FILE_RESULTS;
    
    echo "Generating random bad block in" $RANDOM_BB_PARTITION_C >>$FILE_RESULTS;
    RANDOM_ADDRESS_C=`bbtmod -r $RANDOM_BB_PARTITION_C | grep "random_blockstart = " | cut -c 21-28`;
    echo "Random address C: " $RANDOM_ADDRESS_C >>$FILE_RESULTS;

    echo "-------------------------------" >>$FILE_RESULTS;
    echo "bbt after marking random blocks:" >>$FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_A >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_B >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_C >> $FILE_RESULTS;    
    echo "-------------------------------" >>$FILE_RESULTS;
        
    echo "Erasing BBT:" >>$FILE_RESULTS;
    bbtmod -e;
    echo "Re-scanning BBT:" >>$FILE_RESULTS;
    bbtmod -s;

    echo "-------------------------------" >>$FILE_RESULTS;
    echo "bbt after re-scan:                      " >>$FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_A >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_B >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_C >> $FILE_RESULTS;    
    echo "-------------------------------" >>$FILE_RESULTS;
    
    echo "-------------------------------" >>$FILE_RESULTS;
    echo "Setting the blocks back to GOOD" >>$FILE_RESULTS;
    bbtmod -o 0x${RANDOM_ADDRESS_A} -t G $RANDOM_BB_PARTITION_A;
    bbtmod -o 0x${RANDOM_ADDRESS_B} -t G $RANDOM_BB_PARTITION_B;
    bbtmod -o 0x${RANDOM_ADDRESS_C} -t G $RANDOM_BB_PARTITION_C;

    echo "-------------------------------" >>$FILE_RESULTS;
    echo "bbt after setting blocks back to GOOD:" >>$FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_A >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_B >> $FILE_RESULTS;
    bbtmod -d $RANDOM_BB_PARTITION_C >> $FILE_RESULTS;    
    echo "-------------------------------" >>$FILE_RESULTS;

}


BBT_Auto_Re_Scan_Test_Menu()
{
    while [ 1 ]
    do
        echo "--------------------- Random Bad Block TEst Menu ---------------------"
        echo " BBT auto re-scan test                                                "
        echo " 1) 3 new bad blocks will be created in 3 partitions:                 "
        echo "      a) $RANDOM_BB_PARTITION_A                                       "
        echo "      b) $RANDOM_BB_PARTITION_B                                       "
        echo "      c) $RANDOM_BB_PARTITION_C                                       "
        echo " 2) Then, the BBT will be erased, re-scanned and printed in           "
        echo "    $FILE_RESULTS                                                     "
        echo " 3) Please check that the 3 new BB's are there                        "
        echo " 4) Then, the 3 BB's will be returned to their prior state (ok)       "
        echo " 5) The BBT will be (again) printed in $FILE_RESULT                   "
        echo " 6) Please check that the 3 BB's aren't there anymore.                "
        echo "----------------------------------------------------------------------"          
        echo "[0] Change Partition A Name. Make sure it has been erased prior to this test."
        echo "[1] Change Partition B Name. Make sure it has been erased prior to this test."
        echo "[2] Change Partition C Name. Make sure it has been erased prior to this test."
        echo "[3] Start Test"    
        echo "[4] Return to Main Menu"
        echo "======================================================================"
        echo -n "Enter your menu choice [0 to 4]: "
        read yourch
        case $yourch in
           0) echo "Partition name: "; 
              read RANDOM_BB_PARTITION_BACKUP_A;
              if test "$RANDOM_BB_PARTITION_BACKUP_A" != ""; then
                 RANDOM_BB_PARTITION_A=$RANDOM_BB_PARTITION_BACKUP_A
              fi;;
           1) echo "Partition name: "; 
              read RANDOM_BB_PARTITION_BACKUP_B;
              if test "$RANDOM_BB_PARTITION_BACKUP_B" != ""; then
                 RANDOM_BB_PARTITION_B=$RANDOM_BB_PARTITION_BACKUP_B
              fi;;
           2) echo "Partition name: "; 
              read RANDOM_BB_PARTITION_BACKUP_C;
              if test "$RANDOM_BB_PARTITION_BACKUP_C" != ""; then
                 RANDOM_BB_PARTITION_C=$RANDOM_BB_PARTITION_BACKUP_C
              fi;;
           3) BBT_Auto_Re_Scan_Test;;
           4) return;;
           *) return;;
        esac
    done
    return    

}


while [ 1 ]
do
    echo "------------------- Nand Test Script Menu ------------------------------"
    echo " Results/Statistics will be saved in "$FILE_RESULTS    
    echo " Number of Iterations for Wear testing: "$NAND_WEAR_TESTS_COUNTER 
    echo " Wear testing will be done on "$WEAR_TEST_NAME1 
    echo " Number of Iterations for Read/Write Disturb testing: "$NAND_READWRITE_TESTS_COUNTER    
    echo " Read/Write Disturb testing will be done on "$READWRITE_TEST_NAME1", "$READWRITE_TEST_NAME2" and "$READWRITE_TEST_NAME3
    echo " File System Throughput Tests will be done on MTD partition:"$THROUGHPUT_MTD_PARTITION_NAME 
    echo " RAW Throughput Tests will be done on MTDBLOCK partition:"$THROUGHPUT_MTDBLOCK_PARTITION_NAME                  
    echo "------------------------------------------------------------------------"
    echo "[0] Change the Name of the Results/Statistics File" 
    echo "[1] Other Configurable Parameters Menu"  
    echo "[2] Show Actual NAND Informations"
    echo "[3] Show Actual Bad Block Table Content"
    echo "[4] Generate ECC Error Menu"
    echo "[5] Wear Testing Menu"
    echo "[6] Throughput Tests Menu"
    echo "[7] Read/Write Disturb Tests Menu"
    echo "[8] Bad Block Table Modification"
    echo "[9] Start Nand Chip Approval Process (Iterations:"$NAND_WEAR_TESTS_COUNTER")"
    echo "[10] Show Latest Statistics On Nand Approval Process"
    echo "[11] Generate random Bad Block"
    echo "[12] Generate random Bad Block & Auto Re-Scan BBT"
    echo "[13] Help - Details on the tests"   
    echo "[14] Exit/Stop"
    echo "========================================================================="
    echo -n "Enter your menu choice [0 to 14]: "
    read yourch
    case $yourch in
      0) echo "Enter new File Name: " ; read FILE_RESULTS;;  
      1) echo "----------------------------------"; 
         echo "Other Configurable Parameters Menu";
         echo "----------------------------------"; 
         Other_Parameters_Menu;;                  
      2) echo "Nand Info/Statistics";
         nandstat ; 
         echo "Press Any Key To Continue..." ; read;;
      3) bbtmod -d ; echo "Press Any Key To Continue..." ; read;;
      4) echo "-----------------------------------"; 
         echo "Generate ECC Corretable Error Menu";
         echo "----------------------------------"; 
         Generate_ECC_Error_Menu;; 
      5) echo "---------------------------------------------------" >>$FILE_RESULTS;
         echo "Nand Wear Tests Menu" >>$FILE_RESULTS;
         echo "---------------------------------------------------" >>$FILE_RESULTS;
         nandstat >>$FILE_RESULTS; 
         nandinfo >>$FILE_RESULTS;        
         Nand_Wear_Tests_Menu;
         nandinfo >>$FILE_RESULTS;         
         nandstat >>$FILE_RESULTS;;         
      6) echo "-----------------------------"; 
         echo "Throughput Tests Menu";
         echo "-----------------------------"; 
         Throughput_Tests_Menu;;                 
      7) echo "-----------------" >>$FILE_RESULTS; 
         echo "Read/Write Disturb Tests menu" >>$FILE_RESULTS;
         echo "-----------------" >>$FILE_RESULTS; 
         nandstat >>$FILE_RESULTS;          
         ReadWrite_Disturb_Tests_Menu;
         nandstat >>$FILE_RESULTS;          
         echo "Press Any Key To Continue..." ; read;;         
      8) echo "----------------------------------"; 
         echo "Bad Block Table Modifications Menu";
         echo "----------------------------------"; 
         Bad_Block_Table_Modifications_Menu;;
      9) echo "--------------------------" >>$FILE_RESULTS;  
         echo "Nand Chip Approval Process"  >>$FILE_RESULTS;
         echo "--------------------------" >>$FILE_RESULTS;   
         echo "1- Nand Statistics"          >>$FILE_RESULTS;
         echo "--------------------------" >>$FILE_RESULTS;                 
         nandstat  >>$FILE_RESULTS;
         echo "--------------------------" >>$FILE_RESULTS;   
         echo "2- Bad Block Table Display"  >>$FILE_RESULTS;
         echo "--------------------------" >>$FILE_RESULTS;            
         bbtmod -d >>$FILE_RESULTS;
         echo "--------------------------------------------------" >>$FILE_RESULTS;   
         echo "3- Generate ECC Correctable Error                 " >>$FILE_RESULTS;
         echo "--------------------------------------------------" >>$FILE_RESULTS;            
         Generate_ECC_Correctable_Error;
         echo "--------------------------------------------------" >>$FILE_RESULTS;            
         echo "4- Generate ECC Uncorrectable Error                 " >>$FILE_RESULTS;
         echo "--------------------------------------------------" >>$FILE_RESULTS;            
         Generate_ECC_Uncorrectable_Error;
         echo "--------------------------------------------------" >>$FILE_RESULTS;   
         echo "5- Random BB Test                                 " >>$FILE_RESULTS;   
         echo "--------------------------------------------------" >>$FILE_RESULTS;   
         Random_BB_Test;
         echo "--------------------------------------------------" >>$FILE_RESULTS;   
         echo "6- BBT_Auto_Re_Scan_Test                          " >>$FILE_RESULTS;   
         echo "--------------------------------------------------" >>$FILE_RESULTS;   
         BBT_Auto_Re_Scan_Test;
         echo "--------------------------------------------------" >>$FILE_RESULTS;   
         echo "7- Throughput Tests with NO File System" >>$FILE_RESULTS;
         echo "---------------------------------------" >>$FILE_RESULTS;                       
         Throughput_Tests_with_NO_File_System;  
         echo "------------------------------------" >>$FILE_RESULTS;   
         echo "8- Throughput Tests with File System" >>$FILE_RESULTS;
         echo "------------------------------------" >>$FILE_RESULTS;                         
         Throughput_Tests_with_File_System;      
         echo "--------------------------------------------------" >>$FILE_RESULTS;
         echo "9- Nand Wear Tests                                " >>$FILE_RESULTS;
         echo "--------------------------------------------------" >>$FILE_RESULTS;
         nandstat >>$FILE_RESULTS;                     
         Nand_Wear_Tests;  
         nandstat >>$FILE_RESULTS;          
         echo "11- Nand Statistics"          >>$FILE_RESULTS;
         echo "---------------------------" >>$FILE_RESULTS;                          
         nandstat  >>$FILE_RESULTS;         
         echo "---------------------------" >>$FILE_RESULTS;   
         echo "12- Bad Block Table Display"  >>$FILE_RESULTS;
         echo "---------------------------" >>$FILE_RESULTS;                
         bbtmod -d >>$FILE_RESULTS;          
         echo "Press Any Key To Continue..." ; read;;             
      10)cat $FILE_RESULTS ; echo "Press Any Key To Continue..." ; read ;;
      11)Random_BB_Test_Menu ; echo "Press Any Key To Continue..." ; read ;;
      12)BBT_Auto_Re_Scan_Test_Menu ; echo "Press Any Key To Continue..." ; read ;;
      13)echo "This will call the help/doc file" ; echo "Press Any Key To Continue..." ; read ;;           
      14)exit 0 ;;
       *)exit 0 ;;
    esac
done

#         echo "-------------------" >>$FILE_RESULTS;   
#         echo "9- Read Disturb Test" >>$FILE_RESULTS;
#         echo "--------------------" >>$FILE_RESULTS;                       
#         Read_Disturb_Test;  
#         echo "-------------------" >>$FILE_RESULTS;   
#         echo "10- Write Disturb Test" >>$FILE_RESULTS;
#         echo "--------------------" >>$FILE_RESULTS;                       
#         Write_Disturb_Test;                  
#         echo "---------------------------" >>$FILE_RESULTS;   
