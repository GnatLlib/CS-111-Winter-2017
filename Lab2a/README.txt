This package contains:

Four C source modules that compile cleanly (with no errors or warnings):
	lab2_add.c - a C program that implements and tests a shared variable add function.
	SortedList.h - a header file (supplied by us) describing the interfaces for linked list operations.
	SortedList.c - a C module that implements insert, delete, lookup, and length methods for a sorted doubly linked list 
	lab2_list.c - a C program that implements the (below) specified command line options and produces the (below) specified output statistics.
A Makefile to build the deliverable programs, output, graphs, and tarball with the following targets:
	build ¦ compile all programs (default target)
	tests ¦ run all (over 200) specified test cases to generate results in CSV files. 
	graphs ¦ use gnuplot(1) and the supplied data reduction scripts to generate the required graphs
	tarball ¦ create the deliverable tarball
	clean ¦ delete all generated programs and output
lab2_add.csv - containing all of your results for all of the Part-1 tests.
lab2_list.csv - containing all of your results for all of the Part-2 tests.
graphs (.png files), created by gnuplot(1) on the above csv files with the supplied data reduction scripts:
For lab2_add
	lab2_add-1.png ...threads and iterations required to generate a failure (with and without yields)
	lab2_add-2.png ¦ Average time per operation with and without yields.
	lab2_add-3.png ¦ Average time per (single threaded) operation vs. the number of iterations.
	lab2_add-4.png threads and iterations that can run successfully with yields under each of the three synchronization methods.
	lab2_add-5.png Average time per (multi-threaded) operation vs. the number of threads, for all four versions of the add function.
For lab2_list
	lab2_list-1.png ¦ average time per (single threaded) unprotected operation vs. number of iterations
	lab2_list-2.png ¦ threads and iterations required to generate a failure (with and without yields).
	lab2_list-3.png ¦ iterations that can run (protected) without failure.
	lab2_list-4.png ¦ (corrected) average time per operation (for unprotected, mutex, and spin-lock) vs. number of threads.
a README.txt file containing:
	descriptions of each of the included files.
	brief (1-4 sentences per question) answers to the analysis question.
	

ANALYSIS QUESTIONS:
2.1.1
A failure occurs when the add function is interrupted at a critical section, and another
thread happens to become scheduled and modifies the shared counter variable. It takes a relatively
large number of iterations before these "collision" will occur regularly. For smaller numbers
of iterations it is more likely that the threads are able to complete their job without being 
interrupted and having a "collision" occur. 

2.1.2
The yield runs take much longer because of the overhead involved in forcing the thread to yield,
which requires the kernel to take over and perform the tasks involved in scheduling a new thread
and switching operation. These procedures take much longer than simply incrementing the counter.
The timings we get using the yield option are not really valid per-operation timings, because it 
it is not only including the time used to perform the operations, but also the time used by 
the system to constantly yield and switch threads.

2.1.3
The average time per operation drops because the overhead of creating the threads, loops, and calling
functions decreases relative to the time actually spent performing operations. This cost should
eventually reach a point where increasing the number of iterations does not really affect the 
average time per operations anymore, at which point we have the "correct" cost.

2.1.4
For a low number of threads, there will likely not be many instances of collisions where one 
thread is interrupted, and another thread wants to enter the critical section and is blocked.
As the number of threads rise, the chances of these collisions occuring increases, and threads
will become blocked more and more often, leading to slower operation times. Spin locks become 
very expensive for a large number of threads, because a spin-locked thread will busy-wait, and
will not yield the processor on its own while trying to acquire a lock, instead requiring the
system timed interrupt to switch execution. So as the number of collision increases, this 
busy-waiting policy becomes very inefficient as blocked processes will constantly use up processor
time without doing anything.

2.2.1
For both part 1 and part 2, the cost per protected operation increases as the number of threads
increases, which is expected as an increased number of threads leads to more collision and thus
more waiting time. However, in part 2, the rate of this effect appears much less drastic. This is 
likely because the operation costs in part 2 are much more expensive in relation to the cost of locking
critical sections when compared to part 1. In part one the operation simply consists of incrementing 
a counter, whereas part 2 contains searching through linked lists. So the cost of using protected
operations will appear less drastic in relation to the cost of the actual operations.

2.2.2
Just like with part 1, spin locks become less and less efficient as the number of threads, and
the number of collisions increase. This is because the mutex protection puts a thread to sleep,
waiting for a signal that the mutex lock is released before it wakes up, effectively removing itself
from the scheduler, while the spin-lock protected threads simply continue trying to get the lock without 
yielding the processor, or taking themselves out of the scheduler. As the number of collision increases, 
this busy waiting policy becomes less and less efficient when compared to the policy of putting
waiting threads to sleep. 


