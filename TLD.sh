#!/bin/bash
FILES=./output/*.out
args=($@)
argsno=$#
if [ $argsno -gt 0 ]
then
    for ((i=0; i < $argsno; i++)) {
        res[i]=0
    }
    for f in $FILES
    do
        while read line
        do
            domain=$line
            set -- $domain
            tld=${1#*.}
            freq=$2
            for ((i=0; i < $argsno; i++)) {
                if [ "${args[$i]}" = "$tld" ]
                then
                    res[i]=$((res[i]+freq))
                fi
            }
        done < $f
    done
    for ((i=0; i < $argsno; i++)) {
        echo ${args[$i]} ${res[$i]}
    }
else
    echo "No arguments given."
fi
