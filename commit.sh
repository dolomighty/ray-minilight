#!/bin/sh
$FIRST && FIRST=false exec xterm -e $0
make clean
git reset
git add .
git status
read -p "commit message: " MSG
[ -n "$MSG" ] || MSG=`date +"%F %T"`
git commit -m "$MSG"
git push --all
