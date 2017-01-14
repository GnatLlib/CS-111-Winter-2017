default:
	@gcc -o lab0 -g lab0.c

check: default
	@echo "Lorem Ipsum" > testin.txt
	@rm -f testout.txt
	@./lab0 --input testin.txt --output testout.txt $> junk.txt
	@cmp testin.txt testout.txt || echo "The input and output files do not match"
	@./lab0 --segfault --catch &> junk.txt ||[ $$? -eq 3 ] || echo "Segfault catching produced wrong return"
	@rm -f testin.txt
	@rm -f testout.txt
	@rm -f junk.txt
	@echo "Tests completed"
clean:
	rm -f *.o lab0 	

dist: default
	rm -f lab0-104521566.tar.gz
	tar -czf lab0-104621566.tar.gz *.c makefile README *.png 
	
