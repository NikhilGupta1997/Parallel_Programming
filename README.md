# Parallel_Programming

This repository contains 4 programs that highlight the importance of parallel programming

1) **TSP.c**

This is the implementation of the famous travelling Salesman Problem. It has been coded in c to make use of the OPENMP library which provides support to run the code on multiple threads. The code implements a genetic algorithm to approximately produce an approximate solution to the problem.

2) **parallel_prefix_sum.c**

This is an implementation of the prefix sum algorithm made parallelized to make use of multiple threads (pthreads) in order to reduce running time of the algorithm.

3) **hypersort.cpp**

This is an implementation of the hypersort algorithm which builds upon the quicksort sorting algorithm. It makes use of multiple threads using the MPI library which is supported in CPP.

4) **matrix_multiplication.cu**

This is an implimentaion of simple matrix-vector multiplication written with CUDA support to enable GPUs. This enables high parallelizability and quick run-time. The code highlights how to initialize and use the CUDA environment.
