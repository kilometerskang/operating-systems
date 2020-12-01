#!/bin/bash

# NAME: Miles Kang
# EMAIL: milesjkang@gmail.com
# ID: 405106565

rm -f lab2_add.csv lab2_list.csv

#
# ADD TESTS
#

threads_one=(1 2 4 8 16)
iterations_one=(100 1000 10000 100000)

threads_two=(1 2 4 8 12)
iterations_two=(10 20 40 80 100 1000 10000 100000)

# without sync flag.

for i in ${threads_one[*]}; do
    for j in ${iterations_one[*]}; do
        ./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
    done
done

for i in ${threads_two[*]}; do
    for j in ${iterations_two[*]}; do
        ./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
    done
done

for i in ${threads_two[*]}; do
    for j in ${iterations_two[*]}; do
        ./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
    done
done

# sync flag.

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=100 --sync=m >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=100 --sync=c >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=100 --sync=s >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=1000 --sync=m >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=1000 --sync=c >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=1000 --sync=s >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=10000 --sync=m >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=10000 --sync=c >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=100 --sync=m --yield >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=100 --sync=c --yield >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=100 --sync=s --yield >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=1000 --sync=m --yield >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=1000 --sync=c --yield >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=1000 --sync=s --yield >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=10000 --sync=m --yield >> lab2_add.csv
done

for i in ${threads_two[*]}; do
    ./lab2_add --threads=$i --iterations=10000 --sync=c --yield >> lab2_add.csv
done

#
# LIST TESTS
#

iterations_one=(10 100 1000 10000 20000)

threads_two=(2 4 8 12)
iterations_two=(1 10 100 1000)

threads_thr=(2 4 8 12)
iterations_thr=(1 2 4 8 16 32)

yields=(i d l id il dl idl)

threads_four=(1 2 4 8 12 16 24)

for i in ${iterations_one[*]}; do
    ./lab2_list --threads=1 --iterations=$i >> lab2_list.csv
done

for i in ${threads_two[*]}; do
    for j in ${iterations_two[*]}; do
        ./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
    done
done

for i in ${threads_thr[*]}; do
    for j in ${iterations_thr[*]}; do
        for k in ${yields[*]}; do
            ./lab2_list --threads=$i --iterations=$j --yield=$k >> lab2_list.csv
        done
    done
done

for k in ${yields[*]}; do
    ./lab2_list --threads=12 --iterations=32 --yield=$k --sync=m >> lab2_list.csv
done

for k in ${yields[*]}; do
    ./lab2_list --threads=12 --iterations=32 --yield=$k --sync=s >> lab2_list.csv
done

for i in ${threads_four[*]}; do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2_list.csv
done

for i in ${threads_four[*]}; do
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2_list.csv
done