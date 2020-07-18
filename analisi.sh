#!/bin/bash

while [ -e /proc/$1 ]
do
    sleep 1s
done

if [ -f "data.txt" ]; then
    while read line; do echo $line; done < data.txt
else
    echo "$0:Errore lettura file" 1>&2
fi
