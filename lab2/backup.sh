#!/bin/bash
Secs=$3

# get the filename from the path
File=${1##*/}

NewName="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$File"

cp $File $NewName
Count=1
Mod=($Count)%$4
# create a history array
History[$Mod]=$NewName

while true; do
    sleep $Secs

    if ! cmp -s $1 $NewName; then

        # email user the difference
        echo `diff $1 $NewName` > diff.txt
        /usr/bin/mailx -s "Backup" $USER < diff.txt
        rm diff.txt
        latest="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$File"

        cp $1 $latest
        let Count=$Count+1
        let Mod=($Count)%$4
        NewName=$latest

        # if the count reaches the limit,
        # then update the oldest backup with
        # the latest one.
        if [ $Count -gt $4 ]; then
            rm "${History[$Mod]}"
        fi
        History[$Mod]=$NewName
    fi
done
