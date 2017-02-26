#include "SortedList.h"
#include<stdlib.h>
#include<stdio.h>
#include<getopt.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include<string.h>

int syncopt;
int numLists;
typedef struct thread_datas{
	SortedListElement_t * k;
	long mutex_time;
}thread_data;
//global mutex
pthread_mutex_t m;
pthread_mutex_t ms[100];
//tracker for time spent waiting for mutex

//spin lock implementation
volatile int s_lock;
volatile int s_locks[100];
void set_lock(int l){
	while(__sync_lock_test_and_set(&s_locks[l],1)){
	}

}

void reset_lock(int l){
	__sync_lock_release(&s_locks[l],1);
}
//declare global list
SortedList_t globalList;
SortedList_t globalLists[100];

//global variables to keep track of options
int opt_yield = 0;
int iters;
int thrds;

//calculate elapsed time
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
//job for each thread to run
void *doJob(void *it)
{
	
	thread_data * t = it;
	SortedListElement_t * start = t->k;
	SortedListElement_t * temp;
	int key;
	//if no sync option is selected
	if(syncopt == 0){
	for(int i = 0; i<iters; i++){
		
		key = (int)(*((start+i)->key));
		
		SortedList_insert(&globalLists[key%numLists], start+i);
	
	}
	
	for(int i = 0 ; i<numLists; i++){
	if(SortedList_length(&globalLists[i]) == -1){
		fprintf(stderr, "Corrupted list discovered when calculating length\n");
		
		exit(1);
		
		}
	}
	for(int i = 0; i<iters; i++){
		key = (int)(*((start+i)->key));
		temp = SortedList_lookup(&globalLists[key%numLists], (start+i)->key);
		if(temp == NULL){
			fprintf(stderr, "Inserted element not found!\n");
			exit(1);
		}
		
		
		if(SortedList_delete(temp) == 1){
			fprintf(stderr, "Corruped list discovered when deleting!\n");
			exit(1);
		}
		
	}
	}
	//pthread_mutex implementation
	else if(syncopt == 1){
	struct timespec stime, etime;
		

		
		
	for(int i = 0; i<iters; i++){
		key = (int)(*((start+i)->key));
		
			clock_gettime(CLOCK_MONOTONIC, &stime);
			pthread_mutex_lock(&ms[key%numLists]);
			clock_gettime(CLOCK_MONOTONIC, &etime);
			long ttime = elapsedTime(stime, etime);	
			t->mutex_time += ttime;

		
		SortedList_insert(&globalLists[key%numLists], start+i);
			pthread_mutex_unlock(&ms[key%numLists]);
		
			
	}
	for(int i = 0 ; i<numLists; i++){
	clock_gettime(CLOCK_MONOTONIC, &stime);
			pthread_mutex_lock(&ms[i]);
			clock_gettime(CLOCK_MONOTONIC, &etime);
			long ttime = elapsedTime(stime, etime);	
			t->mutex_time += ttime;
	
	if(SortedList_length(&globalLists[i]) == -1){
		fprintf(stderr, "Corrupted list discovered when calculating length\n");
		
		exit(1);
		
		}
	
	
	pthread_mutex_unlock(&ms[i]);
	}
	
	for(int i = 0; i<iters; i++){
		key = (int)(*((start+i)->key));
		clock_gettime(CLOCK_MONOTONIC, &stime);
			pthread_mutex_lock(&ms[key%numLists]);
			clock_gettime(CLOCK_MONOTONIC, &etime);
			long ttime = elapsedTime(stime, etime);	
			t->mutex_time += ttime;
		
		temp = SortedList_lookup(&globalLists[key%numLists], (start+i)->key);
		
		
		if(temp == NULL){
			fprintf(stderr, "Inserted element not found!\n");
			exit(1);
		}
		
		
		
		if(SortedList_delete(temp) == 1){
			fprintf(stderr, "Corrupted list discovered when deleting!\n");
			exit(1);
		}
		pthread_mutex_unlock(&ms[key%numLists]);
	
	}
	
		
	}
	
	//spinlock implementation
	else if(syncopt == 2){
		
		
		for(int i = 0; i<iters; i++){
			key = (int)(*((start+i)->key));
			set_lock(key%numLists);
			
		
			SortedList_insert(&globalLists[key%numLists], start+i);
			reset_lock(key%numLists);
			
	}
	
	for(int i = 0 ; i<numLists; i++){
	set_lock(i);
	if(SortedList_length(&globalLists[i]) == -1){
		fprintf(stderr, "Corrupted list discovered when calculating length\n");
		
		exit(1);
		
		}
	reset_lock(i);
	}
	
	for(int i = 0; i<iters; i++){
		key = (int)(*((start+i)->key));
		set_lock(key%numLists);
		
		temp = SortedList_lookup(&globalLists[key%numLists], (start+i)->key);
		
		
		if(temp == NULL){
			fprintf(stderr, "Inserted element not found!\n");
			exit(1);
		}
		
		
		if(SortedList_delete(temp) == 1){
			fprintf(stderr, "Corrupted list discovered when deleting!\n");
			exit(1);
		}
		
		reset_lock(key%numLists);
	}
		
	}
	
	
	pthread_exit(NULL);
}
int main(int argc, char ** argv)
{
	srand(time(0));
	iters = 1;
	thrds = 1;
	struct timespec stime, etime;
	char name[100];
	int yielding = 0;
	syncopt = 0;
	opt_yield = 0;
	char yieldopts[100];
	int yieldselected = 0;
	char syncopts[100];
	int syncselected = 0;
	numLists=1;
	
	static struct option long_options[] = 
	{
		{"iterations", required_argument, 0 ,'i'},
		{"threads", required_argument, 0, 't'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{"lists", required_argument, 0, 'l'},
		{0, 0, 0, 0},
	};
	
	
	//parse options
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
			case 'l':{
				numLists = atoi(optarg);
				break;
			}
			case 'y':{
				
				yieldselected = 1;
				strcpy(yieldopts, "-");
				strcat(yieldopts, optarg);
				
				if(strcmp(optarg,"i")==0){
					opt_yield |= INSERT_YIELD;
					
				}
				else if(strcmp(optarg,"d")==0){
					opt_yield |= DELETE_YIELD;
				}
				else if(strcmp(optarg,"l")==0){
					opt_yield |= LOOKUP_YIELD;
				}
				else if(strcmp(optarg, "id")==0){
					opt_yield |= INSERT_YIELD;
					opt_yield |= DELETE_YIELD;
				}
				else if(strcmp(optarg,"il")==0){
					opt_yield |= INSERT_YIELD;
					opt_yield |= LOOKUP_YIELD;
				}
				else if(strcmp(optarg, "dl")==0){
					opt_yield |= DELETE_YIELD;
					opt_yield |= LOOKUP_YIELD;
				}
				else if(strcmp(optarg, "idl")==0){
					opt_yield |= INSERT_YIELD;
					opt_yield |= DELETE_YIELD;
					opt_yield |= LOOKUP_YIELD;
				}
				break; 
			}
			case 's':{
				
				syncselected = 1;
				strcpy(syncopts, "-");
				
				strcat(syncopts, optarg);
				
				
				if (strcmp(optarg,"m")==0){
					
					syncopt = 1;
					break;
				}
				else if (strcmp(optarg,"s")==0){
					
					syncopt = 2;
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
	//initialize list 
	globalList.next = NULL;
	globalList.prev = NULL;
	globalList.key = NULL;

	//initialize lists
	for(int i = 0; i<numLists; i++){
		globalLists[i].next=NULL;
		globalLists[i].prev = NULL;
		globalLists[i].key = NULL;
	}
	

	
	//generate thrds*iters SortedListElement_t objecst with random key
	SortedListElement_t elementList[thrds*iters];
	for(int i = 0; i<(thrds*iters); i++)
	{
		elementList[i].next = NULL;
		elementList[i].prev = NULL;
		char* k = malloc(sizeof(*k));
		*k = "ABCEDFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[rand()%52];
		elementList[i].key = k;
	}
	
	thread_data *my_threaddatas = malloc(sizeof(thread_data)*thrds);
	for (t = 0; t<thrds; t++){
		my_threaddatas[t].k = &elementList[t*iters];
		my_threaddatas[t].mutex_time = 0;
	}
	
	//get initial time
	clock_gettime(CLOCK_MONOTONIC, &stime);
	
	
	//make and run threads
	for(t=0; t<thrds;t++){
		//each threads gets a pointer to an element in elementList which can be used as subarray
		errnum = pthread_create(&threads[t], NULL, doJob, (void *)&my_threaddatas[t]);
		if (errnum){
			printf("Error creating pthreads, errcode: %d\n", errnum);
			exit(-1);
		}
	}
	//join threads and get mutex wait times
	
	for(t=0; t<thrds;t++){
		
		errnum = pthread_join(threads[t], NULL);
	}
	
	
	//end time
	clock_gettime(CLOCK_MONOTONIC, &etime);
	//gather mutex_ttime
	long mutex_ttime = 0;
	for(t=0; t<thrds;t++){
		
		mutex_ttime += my_threaddatas[t].mutex_time;
	}
	if(SortedList_length(&globalList)!=0){
		fprintf(stderr,"Final list length is not 0\n");
		fprintf(stderr, "%d",SortedList_length(&globalList) );
		exit(1);
	}
	
	//generate correct name
	strcpy(name, "list");
	if (yieldselected){
		strcat(name, yieldopts);
	}
	else{
		strcat(name, "-none");
	}
	
	
	if (syncselected){
		strcat(name, syncopts);
	}
	else{
		strcat(name, "-none");
		}
	
	//calculate elapsed time
	

	long ttime = elapsedTime(stime, etime);
	
	long ops = (long)iters * thrds * 3;
	
	printf("%s,%d,%d,%d,%d,%ld,%ld, %ld\n",name,thrds,iters,numLists,ops,ttime,ttime/ops,mutex_ttime/(thrds*iters*2+thrds));
	
	
	pthread_exit(NULL);

}