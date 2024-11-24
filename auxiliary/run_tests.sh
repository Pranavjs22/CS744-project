#!/bin/bash

# Array of number of threads
threads=(1 2 5 10 20 30 50 70 100 130 150 200 250 300 350 500 750 1000)

# Compile the code
gcc -o test test.c emufs-disk.c emufs-ops.c -lpthread

# Create output files
single_threaded_output="single_threaded_output.txt"
multithreaded_output="multithreaded_output.txt"
rm -f $single_threaded_output $multithreaded_output

# Run tests for each number of threads
for num_threads in "${threads[@]}"; do
    echo "Running test with $num_threads threads..."
    rm -f disk*
    ./test $num_threads > temp_output.txt

    # Extract execution times
    single_threaded_time=$(grep "Single-threaded execution time" temp_output.txt | awk '{print $4}')
    multithreaded_time=$(grep "Multithreaded execution time" temp_output.txt | awk '{print $4}')

    # Store the results
    echo "$num_threads $single_threaded_time" >> $single_threaded_output
    echo "$num_threads $multithreaded_time" >> $multithreaded_output
done

# Clean up
rm -f temp_output.txt