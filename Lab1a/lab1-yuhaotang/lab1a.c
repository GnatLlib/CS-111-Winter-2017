#include<getopt.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<ctype.h>
#include<errno.h>



static int verb_flag;
static int fds[2000];

/*storage for pids*/
int my_pids[2000];

/*storage for commands */
char* cmd_args[10];



int isOption(char * s){
	if ((s[0] == '-' )&&(s[1] == '-'))
		return 1;
	else
		return 0;
}
int main(int argc, char ** argv)
{
	
	static struct option long_options[] =
	{
		{"rdonly", required_argument, 0, 'r'},
		{"wronly", required_argument, 0, 'w'},
		{"command", required_argument, 0, 'c'},
		{"verbose", no_argument, 0, 'v'},
		{NULL,0,NULL,0}
		
		
		
	};
	
	int cmd_index;
	int pid_index = 0;
	int opt = 0;
	int verbose_flag = 0;
	int open_flags = 0;
	int fd_index = 0;
	char* verbose_option;
	
	pid_t pid;
	pid_t com_pid;
	
	
	while(1){
		
		int opt = getopt_long(argc, argv, "r:w:vc:", long_options, NULL);
		opterr = 0;
		if (opt==-1)
			break;
		
		switch(opt){
			case 'r':
			case 'w':
				if(opt == 'r'){
					open_flags |= O_RDONLY;
					verbose_option = "RD_ONLY";
				}
				else if(opt == 'w'){
					open_flags |= O_WRONLY;
					verbose_option = "WR_ONLY";
				}
				if(verb_flag){
					
					printf("--%s %s\n", verbose_option , optarg);
				}
				int test_fd = open(optarg, open_flags);
					
				if(test_fd == -1){
					fprintf(stderr, "Error, file %s does not exist.\n", optarg);
					exit(EXIT_FAILURE);
				}
				
				fds[fd_index] = test_fd;
				fd_index ++;
				open_flags = 0;
				break;
			case 'v':
				verb_flag = 1;
				break;
			
			case 'c':
			{
				//clear cmd_args 
				cmd_index = 0;
				int i;
				for(i=0; i<10;i++)
					cmd_args[i] == NULL;
				
				
				int current = optind -1;
				
				//parse arguments for cmd_args
				while((current != argc) && !isOption(argv[current])){
					cmd_args[cmd_index]=argv[current];
					cmd_index++;
					current++;
				}
				
				
				
				//print out option if verbose is set
				if(verb_flag){
					printf("--command");
					int i;
					for(i = 0; i<cmd_index; i++){
						printf(" %s", cmd_args[i]);
					}
					
				printf("\n");
				}
				
				
				
				int pid = fork();
				if(pid ==0){
					
					//point command file descriptors and check to make sure they are valid
					int t1 = dup2(fds[atoi(cmd_args[0])], STDIN_FILENO);
					int t2 = dup2(fds[atoi(cmd_args[1])], STDOUT_FILENO);
					int t3 = dup2(fds[atoi(cmd_args[2])], STDERR_FILENO);
					
					execvp(cmd_args[3], &cmd_args[3]);
					_exit(errno);
				
				}
				
				else if (pid == -1){
					fprintf(stderr, "Error forking process");
					exit(errno);
				}
					
				else {
					
					//check for invalid file descriptors
					for(i=0; i<3;i++){
						if (atoi(cmd_args[i]) >= fd_index){
							fprintf(stderr, "%s is not a valid fd",cmd_args[i]);
						}
					}
					
					my_pids[pid_index] = pid;
					pid_index++;
				}
				
				
				
				
				
				
				optind = current-1;
				break;
				
				
			}
			case ':':
				fprintf(stderr, "Option %s requires an operand/n", verbose_option);
				break;
			
			default:
				fprintf(stderr, "Invalid Option %d entered.", opt);
				break;
				
				
					
		
		}
		
	
	
	}
	int i;
	for(i = 0; i<fd_index; i++){
		if (fds[i] != -1){
			if(close(fds[i])!=0){
				fprintf(stderr, "File could not be closed\n");
				exit(EXIT_FAILURE);
			}
			
			
		}
	}
	
	return(0);
}