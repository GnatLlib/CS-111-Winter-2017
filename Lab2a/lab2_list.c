#include "SortedList.h"
#include<stdlib.h>
#include<stdio.h>
#include<getopt.h>
#include<unistd.h>
#include<pthread.h>
#include<time.h>
#include<string.h>

int syncopt;

//global mutex
pthread_mutex_t m;

//spin lock implementation
volatile int s_lock;
void set_lock(){
	while(__sync_lock_test_and_set(&s_lock,1)){
	}

}

void reset_lock(){
	__sync_lock_release(&s_lock);
}
//declare global list
SortedList_t globalList;

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
	
	SortedListElement_t * start = it;
	SortedListElement_t * temp;
	
	//if no sync option is selected
	if(syncopt == 0){
	for(int i = 0; i<iters; i++){
		
		SortedList_insert(&globalList, start+i);
	
	}
	
	
	if(SortedList_length(&globalList) == -1){
		fprintf(stderr, "Corrupted list discovered when calculating length\n");
		
		exit(1);
		
		}
	for(int i = 0; i<iters; i++){
		temp = SortedList_lookup(&globalList, (start+i)->key);
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
		
		pthread_mutex_lock(&m);
		for(int i = 0; i<iters; i++){
			
			SortedList_insert(&globalList, start+i);
			
			
	}
	
	if(SortedList_length(&globalList) == -1){
		fprintf(stderr, "Corrupted list discovered when calculating length\n");
		
		exit(1);
		
		}
	
	for(int i = 0; i<iters; i++){
	
		temp = SortedList_lookup(&globalList, (start+i)->key);
		
		
		if(temp == NULL){
			fprintf(stderr, "Inserted element not found!\n");
			exit(1);
		}
		
		
		if(SortedList_delete(temp) == 1){
			fprintf(stderr, "Corrupted list discovered when deleting!\n");
			exit(1);
		}
		
	
	}
		pthread_mutex_unlock(&m);
	}
	
	//spinlock implementation
	else if(syncopt == 2){
		
		set_lock();
		for(int i = 0; i<iters; i++){
			
			SortedList_insert(&globalList, start+i);
			
			
	}
	
	if(SortedList_length(&globalList) == -1){
		fprintf(stderr, "Corrupted list discovered when calculating length\n");
		
		exit(1);
		
		}
	
	for(int i = 0; i<iters; i++){
	
		temp = SortedList_lookup(&globalList, (start+i)->key);
		
		
		if(temp == NULL){
			fprintf(stderr, "Inserted element not found!\n");
			exit(1);
		}
		
		
		if(SortedList_delete(temp) == 1){
			fprintf(stderr, "Corrupted list discovered when deleting!\n");
			exit(1);
		}
		
	
	}
		reset_lock();
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
	
	static struct option long_options[] = 
	{
		{"iterations", required_argument, 0 ,'i'},
		{"threads", required_argument, 0, 't'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
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
	
	//generate thrds*iters SortedListElement_t objecst with random key
	SortedListElement_t elementList[thrds*iters];
	for(int i = 0; i<(thrds*iters); i++)
	{
		elementList[i].next = NULL;
		elementList[i].prev = NULL;
		char* k = malloc(sizeof(*k));
		*k = "ABCEDFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[rand()%72];
		elementList[i].key = k;
	}
	
	//get initial time
	clock_gettime(CLOCK_MONOTONIC, &stime);
	
	//make and run threads
	for(t=0; t<thrds;t++){
		//each threads gets a pointer to an element in elementList which can be used as subarray
		errnum = pthread_create(&threads[t], NULL, doJob, (void *)&elementList[t*iters]);
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
	
	printf("%s,%d,%d,1,%d,%ld,%ld\n",name,thrds,iters,ops,ttime,ttime/ops);
	
	
	pthread_exit(NULL);

}