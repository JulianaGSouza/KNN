#!/bin/bash

log=log_file.txt
 
printf "KNN Compare \n" > $log

printf "Sequential Version:\n" >> $log
for file_size in "small" "medium" "large"
do
	printf $file_size"\n" >> $log
	./vs0_0_sequencial datasets/$file_size-train.arff datasets/$file_size-test.arff 3 >> $log
done

printf "\nParallel version with OPenMP:\n" >> $log
for file_size in "small" "medium" "large"
do
	printf $file_size" 1 2 4 8 16 32 64 128\n" >> $log
	for i in 1 2 4 8 16 32 64 128
	do
		./vs1_0_OpenMP datasets/$file_size-train.arff datasets/$file_size-test.arff 3 $i >> $log
	done
done

printf "\nParallel version with Threads:\n" >> $log
for file_size in "small" "medium" "large"
do
	printf $file_size" 1 2 4 8 16 32 64 128\n" >> $log
	for i in 1 2 4 8 16 32 64 128
	do
		./vs1_1_threads datasets/$file_size-train.arff datasets/$file_size-test.arff 3 $i >> $log
	done

done

printf "\nParallel version with process - MPI:\n"  >> $log
for file_size in "small" "medium" "large"
do
	printf $file_size" 1 2 4 8 16 32 64 128\n" >> $log
	for i in 1 2 4 8 16 32 64 128
	do
		mpiexec -oversubscribe -np $i vs2_0_MPI datasets/$file_size-train.arff datasets/$file_size-test.arff 3 >> $log
	done
done
