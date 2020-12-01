#!/bin/bash

# NAME: Miles Kang
# EMAIL: milesjkang@gmail.com
# ID: 405106565

rm -f lab2b_list.csv

iterations=(1000)
threads=(1 2 4 8 12 16 24)

for i in ${threads[*]}; do
    for j in ${iterations[*]}; do
        ./lab2_list --threads=$i --iterations=$j --sync=m >> lab2b_list.csv
    done
done

for i in ${threads[*]}; do
    for j in ${iterations[*]}; do
        ./lab2_list --threads=$i --iterations=$j --sync=s >> lab2b_list.csv
    done
done

iterations=(1 2 4 8 16)
threads=(1 4 8 12 16)

for i in ${threads[*]}; do
    for j in ${iterations[*]}; do
        ./lab2_list --threads=$i --iterations=$j --yield=id --lists=4 >> lab2b_list.csv
    done
done

iterations=(10 20 40 80)

for i in ${threads[*]}; do
    for j in ${iterations[*]}; do
        ./lab2_list --threads=$i --iterations=$j --sync=m --yield=id --lists=4 >> lab2b_list.csv
    done
done

for i in ${threads[*]}; do
    for j in ${iterations[*]}; do
        ./lab2_list --threads=$i --iterations=$j --sync=s --yield=id --lists=4 >> lab2b_list.csv
    done
done

iterations=(1000)
threads=(1 2 4 8 12)
lists=(1 4 8 16)

for i in ${threads[*]}; do
    for j in ${iterations[*]}; do
        for k in ${lists[*]}; do
            ./lab2_list --threads=$i --iterations=$j --sync=m --lists=$k >> lab2b_list.csv
        done
    done
done

for i in ${threads[*]}; do
    for j in ${iterations[*]}; do
        for k in ${lists[*]}; do
            ./lab2_list --threads=$i --iterations=$j --sync=s --lists=$k >> lab2b_list.csv
        done
    done
done