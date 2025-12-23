#!/usr/bin/bash
for i in {1..6}
    do
    ./n7 -pra $i -x ../test/sort.noff > ./n7$i.out.txt
    done

