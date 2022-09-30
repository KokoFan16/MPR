#!/bin/bash

while getopts f:c: flag
do
    case "${flag}" in
        f) filename=${OPTARG};;
        c) count=${OPTARG};;
    esac
done

nfile=$(ls -l . | grep $filename\_.*.csv | wc -l);
echo "File name: $filename, $nfile";

if [[ $nfile -eq $count ]] 
then
    cat $filename\_*csv > $filename.csv;
    rm -rf $filename\_*csv 
else
    echo "Didn't find $count files"; 
fi

