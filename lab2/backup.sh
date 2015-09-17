#!/bin/bash
Secs=$3

FileName="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$1"

cp $1 $FileName
Count=1
Mod=($Count)%$4
# create a history array
History[$Mod]=$FileName

while true; do
    sleep $Secs

    if ! cmp -s $1 $FileName; then

        # email user the difference
        echo `diff $1 $FileName` > diff.txt
        /usr/bin/mailx -s "Backup" $USER < diff.txt
        rm diff.txt
        latest="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$1"

        cp $1 $latest
        let Count=$Count+1
        let Mod=($Count)%$4
        FileName=$latest

        # if the count reaches the limit,
        # then update the oldest backup with
        # the latest one.
        if [ $Count -gt $4 ]; then
            rm "${History[$Mod]}"
        fi
        History[$Mod]=$FileName
    fi
done
