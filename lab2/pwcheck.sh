#!/bin/bash
PW="`egrep ".*" $1`"
LENGTH="${#PW}"
SCORE=0
MSG="Error: Password length invalid."

# pw length test
if egrep -q -w '.{6,32}' $1; then
    let SCORE=$SCORE+$LENGTH
    echo "length test passed"
    
    # special char test
    if egrep -q '[#$+%@]' $1; then
        let SCORE=$SCORE+5
        echo "special char test passed"
    fi

    # one number test 
    if egrep -q '[0-9]' $1; then
        let SCORE=$SCORE+5
        echo "one number test passed"
    fi

    # alpha char test
    if egrep -q '[[:alpha:]]' $1; then
        let SCORE=$SCORE+5
        echo "alpha char test passed"     
    fi

    # alphanumeric char repetition test
    if egrep -q '([a-zA-z0-9])\1+' $1; then
        let SCORE=$SCORE-10
        echo "alphanumeric char repetition caught"
    fi

    # consecutive lowercase char test
    if egrep -q '([a-z]){3}' $1; then
        let SCORE=$SCORE-3
        echo "consecutive lowercase char caught"
    fi

    # consecutive uppercase char test
    if egrep -q '([A-Z]){3}' $1; then
        let SCORE=$SCORE-3
        echo "consecutive uppercase char caught"
    fi

    # consecutive number test
    if egrep -q '([0-9]){3}' $1; then
        let SCORE=$SCORE-3
        echo "consecutive number caught"
        echo "Password Score: $SCORE"
    else
        echo "Password Score: $SCORE"
    fi

else
    echo $MSG
fi
