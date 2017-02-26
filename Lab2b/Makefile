default: build

build:
	gcc -lpthread -u/cs/ugrad/tang/lib/libprofiler.so -g -o lab2_list lab2_list.c SortedList.c
	gcc -lpthread -o lab2_add lab2_add.c 

tests: build
	-rm *.csv
	./generate 2>/dev/null
graphs: 
	gnuplot graph.gp

profile: 
	
	LD_PRELOAD=/u/cs/ugrad/tang/lib/libprofiler.so CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.gperf
	pprof --list=doJob ./lab2_list ./raw.gperf >> profile.gperf

tarball: 
	rm -f lab2b-104621566.tar.gz 
	tar -zcvf lab2b-104621566.tar.gz Makefile *.c *.png *.gperf README.txt *.csv generate graph.gp *.h

clean:
	rm -f *.o *.png *.gperf *.csv lab2_add lab2_list
	