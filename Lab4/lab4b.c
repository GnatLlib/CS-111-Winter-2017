#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <mraa/aio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sched.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

char* my_id = "104521566";
static int PORT = 16000;
char *host = "r01.cs.ucla.edu";
int sockfd;
FILE* output;
volatile int running_flag;
volatile int delay;
volatile int unit;
pthread_mutex_t m;
volatile int off_signal;

void error(const char *msg){
    perror(msg);
    exit(0);
}

float convert_temp(int raw)
{
    const int B = 4275;
    float R = 1023.0/((float)raw)-1.0;
    R = 100000.0*R;
    float temp = 1.0/(log(R/100000.0)/B+1/298.15)-273.15;
    if (unit == 0)
        temp = temp * 1.8 + 32;
    return temp;
}

void* read_temps(){
    mraa_aio_context t_sensor;
    t_sensor = mraa_aio_init(0);
    int reading;
    
    if(write(sockfd, my_id, strlen(my_id)) < 0){
        fprintf(stderr, "ERROR: failed to write to socket\n");
    }

    while(1)
    {   
        
        
        if(off_signal)
            exit(0);
        if (running_flag){
            reading = mraa_aio_read(t_sensor);
            time_t cur_time;
            time(&cur_time);
            float fixed_temp = convert_temp(reading);
            struct tm* fixed_time;
            fixed_time = localtime(&cur_time);
            char formatted_time[10];
            memset(formatted_time, 0, 10);
            strftime(formatted_time, 9, "%H:%M:%S", fixed_time);

            printf("%.1f\n", fixed_temp);
            pthread_mutex_lock(&m);
            fprintf(output, "%s %.1f\n", formatted_time, fixed_temp);
            pthread_mutex_unlock(&m);
            char wbuf[256];
            sprintf(wbuf, "%s TEMP=%.1f", my_id, fixed_temp);
            if(write(sockfd, wbuf, strlen(wbuf))<0){
                fprintf(stderr, "ERROR: failed to write to socket");
            }
            fflush(output);
            sleep(delay);
        }
    }
    mraa_aio_close(t_sensor); 
}

void * get_commands(){
    
    while(1)
    {
        char rbuf[256];
        bzero(rbuf, 256);

        if(read(sockfd, rbuf, 256)<0){
            fprintf(stderr, "ERROR: failed to read from socket");
        }

        if(strcmp(rbuf, "START") == 0){
            running_flag = 1;
            pthread_mutex_lock(&m);
            fprintf(output, "%s\n", rbuf);
            fflush(output);
            pthread_mutex_unlock(&m);
        }
        else if(strcmp(rbuf, "STOP") == 0){
            running_flag = 0;
            pthread_mutex_lock(&m);
            fprintf(output, "%s\n", rbuf);
            fflush(output);
            pthread_mutex_unlock(&m);
        }
        else if(strcmp(rbuf, "OFF") == 0){
           
            pthread_mutex_lock(&m);
            fprintf(output, "%s\n", rbuf);
            fflush(output);
            pthread_mutex_unlock(&m);
            fclose(output);
            close(sockfd);
            exit(0);
        }
        else if(strcmp(rbuf, "SCALE=F") == 0){
           
            unit = 0;
            pthread_mutex_lock(&m);
            fprintf(output, "%s\n", rbuf);
            fflush(output);
            pthread_mutex_unlock(&m);
            
        }
        else if(strcmp(rbuf, "SCALE=C") == 0){
           
            unit = 1;
            pthread_mutex_lock(&m);
            fprintf(output, "%s\n", rbuf);
            fflush(output);
            pthread_mutex_unlock(&m);
            
        }
        else if (strncmp(rbuf, "PERIOD=", 7 ) == 0){

            char* raw_value = malloc(sizeof(char)*(strlen(rbuf)-7));
            
            raw_value = rbuf+7;
            
            int value = atoi(raw_value);
            
            if(value >= 1 || value <= 3600){
                delay = value;
                pthread_mutex_lock(&m);
                fprintf(output, "%s\n", rbuf);
                fflush(output);
                pthread_mutex_unlock(&m);

            }
            else{
                pthread_mutex_lock(&m);
                fprintf(output, "%s I\n", rbuf);
                fflush(output);
                pthread_mutex_unlock(&m);
            }
            

        }
        else if(strcmp(rbuf, "DISP Y") == 0 || strcmp(rbuf, "DISP N") == 0){
           
            
            pthread_mutex_lock(&m);
            fprintf(output, "%s\n", rbuf);
            fflush(output);
            pthread_mutex_unlock(&m);
            
        }
        else{
            pthread_mutex_lock(&m);
            fprintf(output, "%s I\n", rbuf);
            fflush(output);
            pthread_mutex_unlock(&m);
        }
    }
}
int main(){

    delay = 3;
    unit = 0;
    running_flag = 1;
    off_signal = 0;

    struct sockaddr_in serv_addr;
    struct hostent * server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd <0)
        error("ERROR opening soccket");
    
    server = gethostbyname(host);
    if (server == NULL){
        fprintf(stderr, "ERROR, no such host \n");
        exit(0);
    }
    
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(PORT);
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting to host");
    
    output = fopen("lab4_2.log", "w");

    pthread_t threads[2];
    pthread_create(&threads[0], NULL, read_temps, NULL);
    pthread_create(&threads[1], NULL, get_commands, NULL);
    pthread_join(threads[0],NULL);
    pthread_join(threads[1],NULL);

    close(sockfd);
    fclose(output);
    exit(0);

}