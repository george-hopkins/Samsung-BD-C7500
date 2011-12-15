#!/bin/sh
DSYM_TEMP="libavc.dsym"
if [ ! -d $1 ];
then
	echo $0 libavc-dir
	exit 1
fi

arm_v7_vfp_le-objdump -T $1./libavformat.so |grep UND|grep -v GCC |grep -v GLIBC |grep -v CXX > $DSYM_TEMP 
arm_v7_vfp_le-objdump -T $1./libavcodec.so |grep UND|grep -v GCC |grep -v GLIBC |grep -v CXX >> $DSYM_TEMP 
arm_v7_vfp_le-objdump -T $1./libavutil.so |grep UND|grep -v GCC |grep -v GLIBC |grep -v CXX >> $DSYM_TEMP 
./mksymlist.sh $DSYM_TEMP $DSYM_TEMP.list
rm $DSYM_TEMP 

