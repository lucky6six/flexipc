#!/bin/sh

# Remove log
sudo rm -f ./exec_log

# Killall screen
sudo killall screen

# Execute expect
$1

# Check Return value
if [ $? -eq 0 ]; then
        echo ""
        echo "Run $1 Succ"
        echo "===============LOG==============="
        cat ./exec_log
        echo "================================="
else
        echo ""
        echo "Run $1 Failed"
        echo "===============LOG==============="
        cat ./exec_log
        echo "================================="
        sleep 1
        exit -1
fi
