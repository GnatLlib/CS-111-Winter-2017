#include<getopt.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>

void add(long long *pointer, long long value){
	long long sum = *pointer + value;
	*pointer = sum;
}

int main(int argc, char ** argv)
{
	int iters;
	int thrds;

	static struct option long_options[] = 
	{
		{"iterations", required_argument, 0 ,'i'},
		{"threads", required_argument, 0, 't'},
		{0, 0, 0, 0},
	};
	
	while(1){
		int opt = getopt_long(argc, argv, "", long_options, NULL);
		if (opt == -1)
			break;
		switch(opt){
			case 'i':{
				iters = atoi(optarg);
				
			}
			
			case 't'"{
				thrds = atoi(optarg);
			}
		}
	}

	printf("iterations = %d\n",  iters);
	printf("threads = %d\n", thrds);
				



}