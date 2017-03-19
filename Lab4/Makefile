default: one two three
	

one:
	gcc -lm -lmraa -o lab4_1 lab4.c 

two:
	gcc -lm -lmraa -lpthread  -o lab4_2 lab4b.c

three:
	gcc -lm -lmraa -lpthread -lssl -lcrypto -o lab4_3 lab4c.c

tarball:
	rm -f lab4-104621566.tar.gz
	tar -zcvf lab4-104621566.tar.gz Makefile *.c README.txt *.log