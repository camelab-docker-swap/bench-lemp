#!/bin/bash
DEV_NUM=2
for j in $(seq 1 4); do
    blkparse -i nvme${DEV_NUM}n$j.blktrace.* -d blktrace-nvme${DEV_NUM}n$j.bin -f "%5T.%9t, %p, %C, %a, %S, %N\n" -o blktrace-nvme${DEV_NUM}n$j.log
done && for j in $(seq 1 4); do
    btt -A -i blktrace-nvme${DEV_NUM}n$j.bin > btt-nvme${DEV_NUM}n$j.output
done