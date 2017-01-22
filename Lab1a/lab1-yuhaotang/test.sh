#!/bin/sh

echo "TESTING TEXT" > testin.txt
touch testout.txt
touch testerr.txt

./simpsh --rdonly testin.txt --wronly testout.txt
if [ $? -eq 0 ]; then
	echo "Simpsh successfully opens RDONLY and WRONLY"
else
	echo "Simpsh failed to open files"
fi

./simpsh --rdonly testin.txt --wronly testout.txt --wronly testerr.txt --command 0 1 2 cat
diff testin.txt testout.txt
if [ $? -eq 0 ]; then
	echo "Simpsh successfully cats files!"
else 
	echo "Simpsh failed to cat files"
fi

./simpsh --rdonly doesnotexist.txt 
if [ $? -eq 1 ]; then	
	echo "Simpsh succesfully exits on invalid file"
else 
	echo "Simpsh did not detect invalid file"
fi

echo "Tests completed"
rm testin.txt testout.txt testerr.txt
