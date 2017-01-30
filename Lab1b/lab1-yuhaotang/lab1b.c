#include<getopt.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<ctype.h>
#include<errno.h>
#include<signal.h>
#include<sys/wait.h>



static int verb_flag;
static int fds[2000];
static int fd_index = 0;

/*storage for pids*/
int my_pids[2000];
int pid_index = 0;
/*storage for commands */
char* called_cmds[1000][1000];
int called_cmd_sizes[1000];
char* cmd_args[10];

/*pipe stuff */
static int pipes[1000];
static int pipe_fd[2];

//Signals
static int signalnum;

void signal_handler(int num)
{
	fprintf(stderr, "Caught signal %d\n", num);
	exit(num);
	
}

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
		{"rdwr", required_argument, 0, 'R'},
		{"command", required_argument, 0, 'c'},
		{"verbose", no_argument, 0, 'v'},
		
		{"append", no_argument, 0, 'a'},
		{"cloexec", no_argument, 0, 'l'},
		{"creat", no_argument, 0, 'C'},
		{"directory", no_argument, 0, 'd'},
		{"dsync", no_argument, 0, 'D'},
		{"excl", no_argument, 0, 'e'},
        {"nofollow", no_argument, 0, 'n'},
        {"nonblock", no_argument, 0, 'N'},
        {"rsync", no_argument, 0, 's'},
        {"sync", no_argument, 0, 'S'},
        {"trunc", no_argument, 0, 't'},
		
		{"close", required_argument, 0, 'x'},
		{"abort", no_argument, 0, 'A' },
		{"wait", no_argument, 0, 'W'},
		{"pipe", no_argument, 0, 'p' },
		{"catch", required_argument, 0, 'k'},
		{"ignore", required_argument, 0, 'i'},
		{"default", required_argument, 0, 'T'},
		{"pause", no_argument,0,'P'},
		{NULL,0,NULL,0}
		
		
		
	};
	
	int cmd_index;
	
	int opt = 0;
	int verbose_flag = 0;
	int open_flags = 0;
	
	char* verbose_option;
	
	pid_t pid;
	pid_t com_pid;
	
	
	while(1){
		
		int opt = getopt_long(argc, argv, "r:w:vc:", long_options, NULL);
		opterr = 0;
		if (opt==-1)
			break;
		
		switch(opt){
			case 'a':{
				open_flags|= O_APPEND;
				if (verb_flag){
					printf("--append\n");
				}
				break;
			}
			
			case 'l':{
				open_flags|= O_CLOEXEC;
				if (verb_flag){
					printf("--cloexec\n");
				}
				break;
			}
			
			case 'C':{
				open_flags|= O_CREAT;
				if (verb_flag){
					printf("--creat\n");
				}
				break;
			}
			
			case 'd':{
				open_flags|= O_DIRECTORY;
				if (verb_flag){
					printf("--directory\n");
				}
				break;
			}
			
			case 'D':{
				open_flags|= O_DSYNC;
				if (verb_flag){
					printf("--dsync\n");
				}
				break;
			}
			
			case 'e':{
				open_flags|= O_EXCL;
				if (verb_flag){
					printf("--excl\n");
				}
				break;
			}
			
			
			case 'n':{
				open_flags|= O_NOFOLLOW;
				if (verb_flag){
					printf("--nofollow\n");
				}
				break;
			}
			
			case 'N':{
				open_flags|= O_NONBLOCK;
				if (verb_flag){
					printf("--nonblock\n");
				}
				break;
			}
			
			case 's':{
				open_flags|= O_RSYNC;
				if (verb_flag){
					printf("--rsync\n");
				}
				break;
			}
			
			case 'S':{
				open_flags|= O_SYNC;
				if (verb_flag){
					printf("--sync\n");
				}
				break;
			}
			
			case 't':{
				open_flags|= O_TRUNC;
				if (verb_flag){
					printf("--trunc\n");
				}
				break;
			}
			
			case 'r':
			case 'R':
			case 'w':
				if(opt == 'r'){
					open_flags |= O_RDONLY;
					verbose_option = "rd_only";
				}
				else if(opt == 'w'){
					open_flags |= O_WRONLY;
					verbose_option = "wr_only";
				}
				else if (opt == 'R'){
					open_flags |= O_RDWR;
					verbose_option = "rd_wr";
					
				}
				if(verb_flag){
					
					printf("--%s %s\n", verbose_option , optarg);
				}
				int test_fd = open(optarg, open_flags);
					
				if(test_fd == -1){
					fprintf(stderr, "Error, file %s does not exist.\n", optarg);
					break;
				}
				
				fds[fd_index] = test_fd;
				pipes[fd_index]=0;
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
					cmd_args[i] = NULL;
				
				
				int current = optind -1;
				
				//parse arguments for cmd_args
				while((current != argc) && !isOption(argv[current])){
					cmd_args[cmd_index]=argv[current];
					
					if (cmd_index>=3){
						called_cmds[pid_index][cmd_index-3]=argv[current];
					}
					cmd_index++;
					current++;
				}
				
				
				
				called_cmd_sizes[pid_index] = cmd_index - 3;
				
				
				
				
				//print out option if verbose is set
				if(verb_flag){
					printf("--command");
					int i;
					for(i = 0; i<cmd_index; i++){
						printf(" %s", cmd_args[i]);
					}
					
				printf("\n");
				}
				
				//check for invalid file descriptors
				for (i=0; i<3;i++)
				{
					if(fds[atoi(cmd_args[i])] == -1)
						fprintf(stderr,"%d is not a valid file descriptor\n", i);
				}
				
				
				int pid = fork();
				if(pid ==0){
					
					
					if (pipes[atoi(cmd_args[0])])
						close(fds[atoi(cmd_args[0])+1]);
					if (pipes[atoi(cmd_args[1])])
						close(fds[atoi(cmd_args[1])-1]);
					if (pipes[atoi(cmd_args[2])])
						close(fds[atoi(cmd_args[2])-1]);
					
					
					
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
					
					
					if (pipes[atoi(cmd_args[0])]){
						close(fds[atoi(cmd_args[0])]);
						fds[atoi(cmd_args[0])] = -1;
					}
					if (pipes[atoi(cmd_args[1])]){
						close(fds[atoi(cmd_args[1])]);
						fds[atoi(cmd_args[1])] = -1;
					}
					if (pipes[atoi(cmd_args[1])]){
						close(fds[atoi(cmd_args[1])]);
						fds[atoi(cmd_args[1])] = -1;
					}
					
					
					my_pids[pid_index] = pid;
					pid_index++;
				
				
				}
				
				
				
				
				
				
				
				
				
				
				optind = current-1;
				break;
				
				
			}
			
			case 'p':{
				
				if(verb_flag)
					printf("--pipe\n");
					
				if(pipe(pipe_fd) == -1)
					fprintf(stderr, "Pipe was unable to be created");
				
				
				
				fds[fd_index] = pipe_fd[0];
				pipes[fd_index] = 1;
				fd_index++;
				fds[fd_index] = pipe_fd[1];
				pipes[fd_index] = 1;
				fd_index++;
				
				
				break;
			}
			
			
			case 'x':{
				if(verb_flag){
					printf("--close %s\n", optarg);
					
				}
				
				int fd_close = atoi(optarg);
				
				if (fd_close-1 > fd_index){
					fprintf(stderr, "File Descriptor %d is invalid",fd_close);
					break;
				}
				if (close(fds[fd_close-1])!=0)
					fprintf(stderr, "File %s is invalid or could not be closed\n", optarg);
				
				fds[fd_close] = -1;
				
			
				break;
				
			}
			case 'A':{
				if(verb_flag)
					printf("--abort\n");
				raise(SIGSEGV);
				break;
			}
			
			case 'W':{
				if (verb_flag)
					printf("--wait\n");
				
				int i;
				int j;
				int k;
				
				int p_status;
				int c_pid;
				
				for(i=0; i<pid_index;i++){
					
					c_pid = waitpid(-1, &p_status, 0);
					printf("%d ", WEXITSTATUS(p_status));
					
					
					
					for(j=0; j<pid_index;j++)
					{
						if (c_pid == my_pids[j]){
							
							for (k=0;k<called_cmd_sizes[j];k++){
								printf("%s ", called_cmds[j][k]);
								
							}
						}
					}
					printf("\n");
				}
				break;
					
			}
			
			case 'k': {
				
				signalnum = atoi(optarg);
				
				if(verb_flag)
					printf("--catch %d\n", signalnum);
				
				signal(signalnum, signal_handler);
				
				break;
			}
				
			case 'i':{
				
				signalnum = atoi(optarg);
				
				if(verb_flag)
					printf("--ignore %d\n", signalnum);
				
				signal(signalnum, SIG_IGN);
				break;
				
			}
			
			case 'T':{
				
				signalnum = atoi(optarg);
				if(verb_flag)
					printf("--default %d\n", signalnum);
				
				signal(signalnum, SIG_DFL);
				break;
			}
			
			case 'P':{
				if(verb_flag)
					printf("--pause");
				pause();
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
				
				fprintf(stderr, "File could not be closed, invalid descriptor\n");
				exit(EXIT_FAILURE);
			}
			
			
		}
	}
	
	return(0);
}