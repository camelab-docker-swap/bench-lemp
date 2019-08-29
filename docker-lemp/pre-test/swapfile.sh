#!/bin/bash

DEV_NUM=2
for NS_NUM in $(seq 1 4); do
    blktrace -d /dev/nvme${DEV_NUM}n${NS_NUM} -o - | blkparse -i - -f "%5T.%9t, %p, %C, %a, %S, %N\n" -o blktrace-nvme${DEV_NUM}n${NS_NUM}.log
done

