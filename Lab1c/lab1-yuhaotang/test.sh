#!/bin/sh


echo ABCDEFG > pipetest.txt
echo abcdefg > pipecheck.txt
touch pipeerr.txt
touch pipeout.txt
./simpsh --rdonly pipetest.txt --wronly pipeerr.txt --pip --wronly pipeout.txt --command 0 3 1 tr A-Z a-z --command 2 4 1 cat --wait

echo "You should see the commands tr A-z a-z and cat above if wait works"

diff pipeout.txt pipecheck.txt
if [ $? == 0 ]
then
    
    echo "Pipe Works!"
else
    echo "Pipe Failed!"
fi


./simpsh --rdonly pipetest.txt --wronly pipeerr.txt --pip --wronly pipeout.txt --abort --command 0 3 1 tr A-Z a-z --command 2 4 1 cat --wait
echo "If abort works you should see a SegFault!"

echo "PLEASE WAIT..."
touch t.txt
touch a.txt
touch b.txt
./simpsh --catch 17 --rdonly t.txt --wronly a.txt --wronly b.txt --command 0 1 2 sleep 10 --wait

if [ $? == 17 ]
then
    
    echo "Catch Works!"
else
    echo "Catch Failed!"
fi

echo "Tests completed"



rm *.txt
