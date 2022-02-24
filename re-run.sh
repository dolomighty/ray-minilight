#!/bin/sh
exec xterm -e "(make re ; make run) 2>&1 | tee .run.out+err"
