#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded add project
#
# input: lab2_add.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # add operations
#	5. run time (ns)
#	6. run time per operation (ns)
#	7. total sum at end of run (should be zero)
#
# output:
#	lab2_add-1.png ... threads and iterations that run (unprotected) w/o failure
#	lab2_add-2.png ... cost per operation of yielding
#	lab2_add-3.png ... cost per operation vs number of iterations
#	lab2_add-4.png ... threads and iterations that run (protected) w/o failure
#	lab2_add-5.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","



set title "Throughput vs Number of Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_1.png'
set key left top
# grep out only successful (sum=0) yield runs
plot \
     "< sed '1,50!d' lab2_add.csv | grep 'add-m,' " using ($2):(1000000000/$6) \
	title 'add-mutex' with linespoints lc rgb 'red', \
     "< sed '1,50!d' lab2_add.csv | grep 'add-s,' " using ($2):(1000000000/$6) \
	title 'add-spinlock' with linespoints lc rgb 'green', \
     "< sed '1,50!d' lab2_list.csv | grep 'list-none-m,' " using ($2):(1000000000/$7) \
	title 'list-mutex' with linespoints lc rgb 'blue', \
     "< sed '1,50!d' lab2_list.csv | grep 'list-none-s,' " using ($2):(1000000000/$7) \
	title 'list-spinlock' with linespoints lc rgb 'gray', \

set title "Average Wait Time and Op Time vs Threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time(ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top
# grep out only successful (sum=0) yield runs
plot \
      "< sed '1,60!d' lab2_list.csv | grep 'list-none-m,' " using ($2):($7) \
	title 'average-op-time' with linespoints lc rgb 'blue', \
     "< sed '1,60!d' lab2_list.csv | grep 'list-none-m,' " using ($2):($8) \
	title 'average-wait-time' with linespoints lc rgb 'gray', \


set title "Uprotected and Protected Multilists that run without failure"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2_3.png'
# note that unsuccessful runs should have produced no output
plot \
      "< grep list-id-none lab2_list.csv" using ($2):($3) \
	title 'no protections' with points lc rgb 'green', \
     "< grep list-id-m lab2_list.csv" using ($2):($3) \
	title 'mutex-protected' with points lc rgb 'red', \
     "< grep list-id-s lab2_list.csv" using ($2):($3) \
	title 'sync-protected' with points lc rgb 'gray', \

set title "Throughput of Mutex-protected Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_4.png'
set key left top
# grep out only successful (sum=0) yield runs
plot \
     "< sed -n '50,$p' lab2_list.csv | grep 'list-none-m,'  | grep ',1000,1,'" using ($2):(1000000000/$7) \
	title '1 List' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,' lab2_list.csv | grep ',1000,4,'" using ($2):(1000000000/$7) \
	title '4 Lists' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,' lab2_list.csv | grep ',1000,8,'" using ($2):(1000000000/$7) \
	title '8 Lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,' lab2_list.csv | grep ',1000,16,'" using ($2):(1000000000/$7) \
	title '16 Lists' with linespoints lc rgb 'gray' 

set title "Throughput of Spinlock-protected Partitioned Lists"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (ops/sec)"
set logscale y 10
set output 'lab2b_5.png'
set key left top
# grep out only successful (sum=0) yield runs
plot \
     "< sed -n '50,$p' lab2_list.csv | grep 'list-none-s,'  | grep ',1000,1,'" using ($2):(1000000000/$7) \
	title '1 List' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,' lab2_list.csv | grep ',1000,4,'" using ($2):(1000000000/$7) \
	title '4 Lists' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,' lab2_list.csv | grep ',1000,8,'" using ($2):(1000000000/$7) \
	title '8 Lists' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,' lab2_list.csv | grep ',1000,16,'" using ($2):(1000000000/$7) \
	title '16 Lists' with linespoints lc rgb 'gray'