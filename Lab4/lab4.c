#include <netdb.h>
#include <sys/time.h>
#include <stdlib.h>
#include <mraa/aio.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

float convert_temp(int raw)
{
    const int B = 4275;
    float R = 1023.0/((float)raw)-1.0;
    R = 100000.0*R;
    float temp = 1.0/(log(R/100000.0)/B+1/298.15)-273.15;
    temp = temp*1.8 + 32;
    return temp;
}
int main(){
    mraa_aio_context t_sensor;
    t_sensor = mraa_aio_init(0);
    int reading;
    FILE* output = fopen("lab4_1.log", "w");
    
    while(1)
    {
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
        
        fprintf(output, "%s %.1f\n", formatted_time, fixed_temp);
        fflush(output);
        sleep(1);
    }

    fclose(output);
    mraa_aio_close(t_sensor);
}