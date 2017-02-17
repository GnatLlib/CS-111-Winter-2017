#include<getopt.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include<string.h>
static long long counter; 
static int opt_yield; //keep track of whether yield is set
static int mutexopt; //keep track of which mutex option ("m,s, or c") is selected
volatile int s_lock; //global spin lock
int iters; //global variable for number of iterations
pthread_mutex_t counter_mutex; //pthread mutex
//calculate difference in times
long elapsedTime(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	
	long ttime = temp.tv_sec * 1000000000 + temp.tv_nsec;
	return ttime;
}
//spin lock implementation
void set_lock(){
	while(__sync_lock_test_and_set(&s_lock,1)){
	}

}

void reset_lock(){
	__sync_lock_release(&s_lock);
}

void add(long long *pointer, long long value) {
				
                long long sum = *pointer + value;
                if (opt_yield)
                        sched_yield();
                *pointer = sum;
        }
void add_mutex(long long *pointer, long long value) {
				pthread_mutex_lock(&counter_mutex);
                long long sum = *pointer + value;
                if (opt_yield)
                        sched_yield();
                *pointer = sum;
				pthread_mutex_unlock(&counter_mutex);
        }
void add_spinlock(long long *pointer, long long value) {
				set_lock();
                long long sum = *pointer + value;
                if (opt_yield)
                        sched_yield();
                *pointer = sum;
				reset_lock();
        }	
void add_atomic(long long *pointer, long long value) {
				
				while(1){
						int temp = *pointer ;
						int next = temp+value;
					if (opt_yield)
                        sched_yield();
					if(__sync_val_compare_and_swap(pointer, temp, next)==temp){
						break;
					}
					
                
				}
        }	
void *doJob()
{
	
	int iter = iters;
	if(mutexopt == 0){
	for (int i = 0; i< iter; i++){
		
		add(&counter, 1);
		}
	for (int i = 0; i< iter; i++){
		add(&counter, -1);
	}
	}
	else if(mutexopt == 1){
	for (int i = 0; i< iter; i++){
		
		add_mutex(&counter, 1);
		}
	for (int i = 0; i< iter; i++){
		add_mutex(&counter, -1);
	}
	}
	else if(mutexopt == 2){
	for (int i = 0; i< iter; i++){
		
		add_spinlock(&counter, 1);
		}
	for (int i = 0; i< iter; i++){
		add_spinlock(&counter, -1);
	}
	}
	else if(mutexopt == 3){
	
	for (int i = 0; i< iter; i++){
		
		add_atomic(&counter, 1);
		}
	for (int i = 0; i< iter; i++){
		add_atomic(&counter, -1);
	}
	}
	pthread_exit(NULL);
}

int main(int argc, char ** argv)
{
	
	iters = 1;
	int thrds = 1;
	struct timespec stime, etime;
	char * name = "add-none";
	int yielding = 0;
	mutexopt = 0;
	counter = 0;
	static struct option long_options[] = 
	{
		{"iterations", required_argument, 0 ,'i'},
		{"threads", required_argument, 0, 't'},
		{"yield", no_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0, 0, 0, 0},
	};
	
	while(1){
		int opt = getopt_long(argc, argv, "", long_options, NULL);
		if (opt == -1)
			break;
		switch(opt){
			case 'i':{
				iters = atoi(optarg);
				break;
			}
			
			case 't':{
				thrds = atoi(optarg);
				break;
			}
			case 'y':{
				opt_yield = 1;
				yielding = 1;
				break;
			}
			case 's':{
				if (strcmp(optarg,"m")==0){
					
					mutexopt = 1;
					break;
				}
				else if (strcmp(optarg,"s")==0){
					mutexopt = 2;
					break;
				}
				else if (strcmp(optarg,"c")==0){
					mutexopt = 3;
					break;
				}
				else {
					printf("INVALID MUTEX OPTION %s",optarg);
					exit(2);
				}
				break;
			}
				
			default:
				break;
		}
	}
	 
	pthread_t threads[thrds];
	int errnum;
	int t;
	
	//get initial time
	clock_gettime(CLOCK_MONOTONIC, &stime);
	
	//make and run threads
	for(t=0; t<thrds;t++){
		errnum = pthread_create(&threads[t], NULL, doJob, NULL);
		if (errnum){
			printf("Error creating pthreads, errcode: %d\n", errnum);
			exit(-1);
		}
	}
	//join threads
	for(t=0; t<thrds;t++){
		errnum = pthread_join(threads[t], NULL);
	}
	
	//end time
	clock_gettime(CLOCK_MONOTONIC, &etime);
	
	//generate correct name
	if(yielding){
		if(mutexopt == 0)
			name = "add-yield-none";
		if(mutexopt == 1)
			name = "add-yield-m";
		if(mutexopt == 2)
			name = "add-yield-s";
		if(mutexopt == 3)
			name = "add-yield-c";
	}
	else{
		if(mutexopt == 0)
			name = "add-none";
		if(mutexopt == 1)
			name = "add-m";
		if(mutexopt == 2)
			name = "add-s";
		if(mutexopt == 3)
			name = "add-c";
	}
	
	//calculate elapsed time
	long ttime = elapsedTime(stime, etime);
	
	long ops = (long)iters * thrds * 2;

	printf("%s,%d,%d,%d,%ld,%ld,%d\n",name,thrds,iters,ops,ttime,ttime/ops,counter);
				

	pthread_exit(NULL);

}