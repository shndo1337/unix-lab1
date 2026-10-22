#!/bin/sh

DIR=/data
LOCK=$DIR/.lock

ID=$(head -c 8 /dev/urandom | od -An -tx1 | tr -d ' \n')
N=0

mkdir -p "$DIR"

while true; do
    N=$((N + 1))

    (
        flock 9
        i=1
        while true; do
            NAME=$(printf '%03d' "$i")
            [ -e "$DIR/$NAME" ] || break
            i=$((i + 1))
        done
        echo "$ID $N" > "$DIR/$NAME"
        echo "$NAME" > "/tmp/name_$ID"
    ) 9>"$LOCK"

    NAME=$(cat "/tmp/name_$ID")
    sleep 1
    rm -f "$DIR/$NAME"
    sleep 1
done
