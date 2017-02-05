all: simpsh

simpsh: 
	$gcc -o simpsh ./lab1c.c
	
clean:
	rm -f simpsh *.o 
	
check: simpsh
	./test.sh
	
dist: simpsh
	rm -f lab1-yuhaotang.tar.gz 
	tar -zcvf lab1-yuhaotang.tar.gz --transform 's,^,lab1-yuhaotang/,' Makefile lab1c.c README test.sh Benchmark.pdf
	
	tar -xvf lab1-yuhaotang.tar.gz
	cd lab1-yuhaotang
	make check
	echo "Package Tested"
	cd ..
	rm -r lab1-yuhaotang 
