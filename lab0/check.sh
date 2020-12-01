#!/bin/bash

#NAME: Miles Kang
#EMAIL: milesjkang@gmail.com
#ID: 405106565

touch input.txt
echo "Hello." >> input.txt
./lab0 --input input.txt --output output.txt

# This should exit successfully.
if [ $? -eq 0 ]
then
    echo "Exited successfully. (0)"
else
    echo "The program did not exit successfully."
fi

# Check that input and output are equal.
cmp -s input.txt output.txt
if [ $? -eq 0 ]
then
    echo "Output file was created successfully."
    echo "Input file was copied successfully to output."
else
    echo "The input file was not copied successfully to output."
fi

# Test an unrecognized option.
./lab0 --what
if [ $? -eq 1 ]
then
    echo "Running with an unrecognized argument exited successfully. (1)"
else
    echo "Unexpected error code when run with an unrecognized argument."
fi

# Test segfault and catch arguments.
./lab0 --segfault --catch
if [ $? -eq 4 ]
then
    echo "Segfault was successfully forced and caught. (4)"
else
    echo "Segfault was not successfully forced or caught."
fi

# Test a nonexistent input file.
./lab0 --input nofile
if [ $? -eq 2 ]
then
    echo "Running with a nonexistent input file exited successfully. (2)"
else
    echo "Unexpected error code when run with a nonexistent input file."
fi

# Test an unmodifiable output file.
chmod u-w output.txt
./lab0 --output output.txt
if [ $? -eq 3 ]
then
    echo "Running with an unmodifiable output file exited successfully. (3)"
else
    echo "Unexpected error code when run with an unmodifiable output file."
fi

# Remove test files.
rm -f input.txt output.txt
