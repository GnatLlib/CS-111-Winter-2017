#include<stdio.h>
#include<unistd.h>
#include<getopt.h>
#include<signal.h>
#include<fcntl.h>
#include<stdlib.h>

void print_correct_use()
{
	printf("Usage: lab0 -i filename -o filename [sc] \n");
}

void signal_handler(int s_num)
{
	if(s_num == SIGSEGV)
	{
		perror("Error: Segmentation Fault Caught");
		exit(3);
	}
}

int main(int argc, char** argv){
	
	static struct option long_options[] =
	{
		{"output", required_argument, 0,'o'},
		{"input", required_argument,0,'i'},
		{"segfault", no_argument, 0, 's'},
		{"catch", no_argument, 0, 'c'},
		{0,0,0,0}
	};
	
	char* in_filename = NULL;
	char* out_filename = NULL;
	char* seg = NULL;
	int seg_fault_flag = 0;
	int ifd = 0;
	int ofd = 1;
	int opt=0;
	
	while(1){
		opt = getopt_long(argc, argv ,"i:o:sc", long_options, NULL);
		if (opt == -1)
			break;
		switch(opt)
		{
			case 'i':
				in_filename = optarg;
				break;
				
			case'o':
				out_filename = optarg;
				break;
			case 's':
				seg_fault_flag = 1;
				break;
			case 'c':
				signal(SIGSEGV, signal_handler);
				break;
			default:
				printf("Please enter correct arguments");
				exit(EXIT_FAILURE);
				}
		}
	
		if(in_filename)
		{
			ifd= open(in_filename, O_RDONLY);
			if (ifd >=0){
				
				close(0);
				dup(ifd);
				close(ifd);
			}
			else{
				
				perror("Error opening file.");
				exit(1);
			}
		}
		if(out_filename){
			
			ofd = creat(out_filename, S_IRWXU);
			if(ofd>=0){
			
				close(1);
				dup(ofd);
				close(ofd);
			}
			else{
				perror("Error creating file.");
				exit(2);
			}
		}
		if(seg_fault_flag){
			*seg = 1;
		}
		
		char buffer[2048] = {};
		int rd_size = 0;
		
		while((rd_size = read(0,buffer,2048))>0){
			write(1, buffer, rd_size);
		}
		return 0;
}
