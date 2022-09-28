#!/bin/bash
nfile=$(ls -l ../build/examples/ | grep .*csv | wc -l);
if [ $nfile > 0 ]; then
    
    echo "$nfile"
else
    echo "0"
fi
#cat *.txt >> all.txt
