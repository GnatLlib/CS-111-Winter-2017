This package contains:
SortedList.h - a header file containing interfaces for linked list operations.
the source for a C source module (SortedList.c)
the source for a C program (lab2_add.c)
the source for a C program (lab2_list.c) 
A Makefile to build the deliverable programs, output, graphs, and tarball. 
lab_2b_list.csv - containing myresults for performance tests.
lab_2b_add.csv - containing generated relevant data
Execution profiling report showing where time was spent :
graphs (.png files), created by gnuplot(1) on the above csv data showing:
	lab2b_1.png … throughput vs number of threads for mutex and spin-lock synchronized adds and list operations.
	lab2b_2.png … mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
	lab2b_3.png … number of successful iterations for each synchronization method.
	lab2b_4.png … throughput vs number of threads for mutexes with partitioned lists.
	lab2b_5.png … throughput vs number of threads for spin-locks with partitioned lists.
a README.txt file containing:
descriptions of each of the included files and answers to analysis questions

IMPORTANT NOTES:

	- Throughout my testing, the spinlock protected implementation tended to perform faster than the mutex protected one,
	  even at high thread counts, which is not the expected result, although there are times where spin-lock does end up 
	  being slower.
	- My implementation of the partitioned list lab2_list can handle up to 100 lists.

ANALYSIS QUESTIONS:

2.3.1 
For the 1-2 thread tests, most of the cycles are probably spent on the actual SortedList functions, namely the insert function and 
lookup functions, which are performed x iterations number of times and involve searching through the linked lists, and the overhead 
due to parallelization is either non-existent or very low.
For high thread spin-lock tests, most of the cycles are likely being used by the set_lock() function as it busywaits trying
to acquire a lock, since every thread that can't acquire a lock when it is scheduled simply hangs there, using up CPU time.
For high thread mutex tests, most of the cycles are likely being spent putting on the mutex_lock() function since the overhead
from using a system call to put threads to sleep and waking them up again is very high when it must be done repeatedly.

2.3.2
The set_lock() function, which involves trying to acquire and busy waiting for the lock is using up by far the majority of the 
cycles, which is expected. This operation becomes so expensive with a large number of threads because it becomes far more likely
that the cpu will schedule a thread that tries to acquire the lock while the lock is already held, and the thread will use all 
of it's cpu time trying to get the lock.

2.3.3
When the number of threads increases, each thread must wait much longer between trying to acquire the lock, and then being woken
up after another thread has given up the mutex. This naturally increases the completion time per operation since more time is 
being spent waiting, which increases the overall time relative to the number of operations. The wait-time per operation goes up 
much faster than the completion time per operation since the wait-time is counting the waits from each thread and adding them all up,
while the completion time only takes into account the longest wait time. For example, if 3 threads are waiting, the wait-time 
per operation counts the wait time 3 times, while the completion time only accounts for this wait time once. 

2.3.4
The throughput of the test is roughly mulitiplied by the number of lists N. This increase in performance is due to the fact that 
partitioning the list and locking each list individually allows for finer grain locking as well as smaller lists, both of which 
result in significant performance increases. With finer-grained locking, instead of having to wait every time to modify one list,
there are multiple lists which can be accessed at the same time, allowing for true parallelization. Also, partitioning the lists
hugely improves the speed of the insert and lookup functions, since it is effectively utilizing a hash table rather than searching
through a linked list. The throughput should stop increasing once the number of lists exceeds 52, since there are only 52 keys,
and our paritioning algorithm allows for a maximum of 52 lists to be utilized. In my tests, the throughput of a N-partitioned
list is actually faster than a single-partitioned list with 1/N threads. This is likely because the partitioning algorithm not only
decreases the wait time by a factor of N, it also greatly increases the speed of the insert and lookup functions, and decreases the
overhead involved in opening and closing locks. 