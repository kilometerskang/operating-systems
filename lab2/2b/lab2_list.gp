#! /usr/bin/gnuplot
#
# NAME: Miles Kang
# EMAIL: milesjkang@gmail.com
# ID: 405106565
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png
set title "List-1: Operations per Second vs. Number of Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_1.png'
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'with mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'with spin lock' with linespoints lc rgb 'green'


# lab2b_2.png
set title "List-2: Wait Time and Time per Operation vs. Number of Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Average Time per Op (ns)"
set logscale y 10
set output 'lab2b_2.png'
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'operation time' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'wait-for-lock time' with linespoints lc rgb 'green'
     

# lab2b_3.png
set title "List-3: Successful Iterations vs. Number of Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
     "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'operation time' with linespoints lc rgb 'red', \
     "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'wait-for-lock time' with linespoints lc rgb 'green', \
     "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'wait-for-lock time' with linespoints lc rgb 'blue'


# lab2b_4.png
set title "List-4: Mutex Throughput vs. Number of Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_4.png'
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'operation time' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'wait-for-lock time' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'wait-for-lock time' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'wait-for-lock time' with linespoints lc rgb 'orange'


# lab2b_5.png
set title "List-5: Spin Lock Throughput vs. Number of Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_5.png'
plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'operation time' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'wait-for-lock time' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'wait-for-lock time' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'wait-for-lock time' with linespoints lc rgb 'orange'