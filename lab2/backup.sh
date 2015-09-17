#!/bin/bash
Secs=$3

FileName="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$1"

cp $1 $FileName
Count=1
Mod=($Count)%$4
History[$Mod]=$FileName

while true; do
    sleep $Secs

    if ! cmp -s $1 $FileName; then
        echo `diff $1 $FileName` > diff.txt
        /usr/bin/mailx -s "Backup" $USER < diff.txt
        latest="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$1"
        cp $1 $latest
        let Count=$Count+1
        let Mod=($Count)%$4
        FileName=$latest
        if [ $Count -gt $4 ]; then
            rm "${History[$Mod]}"
        fi
        History[$Mod]=$FileName
    fi
done
