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
#include <resolv.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

SSL *ssl;
char* my_id = "104521566";
int server;
FILE* output;
volatile int running_flag;
volatile int delay;
volatile int unit;
pthread_mutex_t m;
volatile int off_signal;

int create_socket(char url_str[]) {
  int sockfd;
  char hostname[256] = "";
  char    portnum[6] = "443";
  char      proto[6] = "";
  char      *tmp_ptr = NULL;
  int           port;
  struct hostent *host;
  struct sockaddr_in dest_addr;

  strncpy(proto, url_str, (strchr(url_str, ':')-url_str));
  strncpy(hostname, strstr(url_str, "://")+3, sizeof(hostname));

  if(strchr(hostname, ':')) {
    tmp_ptr = strchr(hostname, ':');
   
    strncpy(portnum, tmp_ptr+1,  sizeof(portnum));
    *tmp_ptr = '\0';
  }

  port = atoi(portnum);

  if ( (host = gethostbyname(hostname)) == NULL ) {
    fprintf(stderr, "Error: Cannot resolve hostname %s.\n",  hostname);
    abort();
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  dest_addr.sin_family=AF_INET;
  dest_addr.sin_port=htons(port);
  dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);


  memset(&(dest_addr.sin_zero), '\0', 8);

  tmp_ptr = inet_ntoa(dest_addr.sin_addr);

  if ( connect(sockfd, (struct sockaddr *) &dest_addr,
                              sizeof(struct sockaddr)) == -1 ) {
    fprintf(stderr, "Error: Cannot connect to host %s [%s] on port %d.\n",
             hostname, tmp_ptr, port);
  }

  return sockfd;
}
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
    
    if(SSL_write(ssl, my_id, strlen(my_id)) < 0){
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
            if(SSL_write(ssl, wbuf, strlen(wbuf))<0){
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

        if(SSL_read(ssl, rbuf, 256)<0){
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
            close(server);
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
    char           dest_url[] = "https://r01.cs.ucla.edu:17000";
    X509                *cert = NULL;
    X509_NAME       *certname = NULL;
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    
    int ret, i;

    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();


    if(SSL_library_init() < 0)
        fprintf(stderr, "Could not initialize the OpenSSL library !\n");

    method = SSLv23_client_method();

    if ( (ctx = SSL_CTX_new(method)) == NULL)
        fprintf(stderr, "Unable to create a new SSL context structure.\n");

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

    ssl = SSL_new(ctx);

    server = create_socket(dest_url);

    SSL_set_fd(ssl, server);

    if ( SSL_connect(ssl) != 1 )
        fprintf(stderr, "Error: Could not build a SSL session to: %s.\n", dest_url);

    cert = SSL_get_peer_certificate(ssl);
    if (cert == NULL)
        fprintf(stderr, "Error: Could not get a certificate from: %s.\n", dest_url);

    output = fopen("lab4_3.log", "w");

    pthread_t threads[2];
    pthread_create(&threads[0], NULL, read_temps, NULL);
    pthread_create(&threads[1], NULL, get_commands, NULL);
    pthread_join(threads[0],NULL);
    pthread_join(threads[1],NULL);

    SSL_free(ssl);
    close(server);
    X509_free(cert);
    SSL_CTX_free(ctx);
   
    fclose(output);
    exit(0);

}