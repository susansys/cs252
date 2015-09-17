#!/bin/bash
Secs=$3

fileName="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$1"
for i in `seq 1 $4`
do
    history[$i]=""
done


cp $1 $fileName                           #copy file into backup dir
count=1
let mod=($count)%$4
history[$mod]=$fileName                   #store first file name into
                                          #history array


while true; do
    sleep $Secs

    if ! cmp -s $1 $fileName; then
        # echo "Difference detected"
        echo `diff $1 $fileName` > diff.txt
        /usr/bin/mailx -s "Backup" $USER < diff.txt
        latest="$2/$(date +"%Y-%m-%d-%H-%M-%S")-$1"
        cp $1 $latest
        let count=$count+1
        let mod=($count)%$4
        fileName=$latest
        if [[ $count -gt $4 ]]; then          #check if need to delete file
            rm "${history[$mod]}"               #before save its name to history
        fi
        history[$mod]=$fileName               #save the latest file name into
    fi
done
