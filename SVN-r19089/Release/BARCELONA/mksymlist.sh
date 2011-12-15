#!/bin/sh

if [ $# -lt 2 ] || [ ! -e $1 ];
then
	echo "usage: $0  Input-file  Output-file"
	exit 1
fi

echo "{" > $2
while read line
do
	var=${line:45}
	echo $var\; >> $2
done < $1
echo "};" >> $2
