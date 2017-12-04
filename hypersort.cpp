#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <string>
#include <ctime>

using namespace std;

// Read from input file
void read_file(char file[], vector<long> &inputs, long &size) {
    fstream in;
    in.open(file);
    if(!in.is_open()) {
        return;
    }
    else {
        long temp;
        in >> size;

        for(long i = 0; i < size; i++) {
            in >> temp;
            inputs.push_back(temp);
        }
    }
    in.close();
}

//Function to swap long values of arr
void swap_pos(vector<long> &arr, long a, long b){
    long temp = arr[b];
    arr[b] = arr[a];
    arr[a] = temp;
}

void print_array(vector<long> &arr){
    long n = arr.size();
    for(long i=0; i<n; i++){
        cout<<arr[i]<<" ";
    }
}

// Quicksort Algorithm
void quicksort(vector<long> &arr, long piv, long high){
    if(piv >= high){
        return 0;
    }

    long pivot = arr[piv];
    long head = piv+1;
    long tail = high;
    while(head < tail){
        if(arr[head] > pivot){
            swap_pos(arr, head, tail);
            tail = tail-1;
        }
        else{
            head = head+1;
        }
    }

    if(arr[head] > pivot){
        swap_pos(arr, piv, head-1);
        quicksort(arr, piv, head-2);
        quicksort(arr, head, high);
    }
    else{
        swap_pos(arr, piv, head);
        quicksort(arr, piv, head-1);
        quicksort(arr, head+1, high);
    }

    return 0;
}

// Returns the median of a vector
long get_median(vector<long> &arr, long size) {
    if(size%2 == 0)
        return arr[size/2-1];
    else 
        return arr[size/2];
}

// Split a vector into 2 vectors
void split(vector<long> &local, vector<long> &left, vector<long> &right, long median, long &size1, long &size2) {
    for(long i = 0; i < local.size(); i++) {
        if(local[i] <= median) {
            left.push_back(local[i]);
        }
        else {
            right.push_back(local[i]);
        }
    }
    size1 = left.size();
    size2 = right.size();
}

// Merge two vectors
void merge(vector<long> &local, vector<long> &left, vector<long> &right) {
    int size = left.size() + right.size();
    int a = 0, b = 0;
    for(int i = 0; i < size; i++) {
        if(a >= left.size()) {
            local.push_back(right[b]);
            b++;
        }
        else if(b >= right.size()) {
            local.push_back(left[a]);
            a++;
        }
        else if(left[a] < right[b]) {
            local.push_back(left[a]);
            a++;
        }
        else {
            local.push_back(right[b]);
            b++;
        }
    }
}

int main(int argc, char** argv) {

    vector<long> inputs;
    long size;
    read_file(argv[1], inputs, size);

    // Create output file name
    string inputfile(argv[1]);
    string prefix = inputfile.substr(5);
    string filen;
    filen += "output";
    filen += prefix;
    char filename[1024];
    strcpy(filename, filen.c_str());

    clock_t begin = clock();

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Declare some general use buffers and variables
    MPI_Status status;
    long source, dest;
    long size_left, size_right, size_local;
    long median, buff;
    vector<long> left, right, temp, local, final;

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    buff = size % world_size;
    size = size/world_size;
    

    if(world_rank == 0) {
        size += buff;
    }

    // Prlong off a hello world message
    printf("Hello world from processor %s, rank %d"
           " out of %d processors size of array is %ld \n",
           processor_name, world_rank, world_size, size);

    // Initiallize the Arrays
    if(world_rank == 0) {
        for(long i = 0; i < size; i++) {
            local.push_back(inputs[i]);
        } 
    }
    else {
        for(long i = world_rank*size + buff; i < (world_rank+1)*size + buff; i++) {
            local.push_back(inputs[i]);
        } 
    }

    // Sort each individual array sequence
    quicksort(local, 0, size-1); 

    for (long pow_level = world_size; pow_level > 1; pow_level /= 2) {
        // Get median of first processor of level
        // Broadcast the value of median to all processors of that level
        if(world_rank % pow_level == 0) {
            median = get_median(local, local.size());
            for(long i = world_rank + 1; i < world_rank + pow_level; i++) {
                MPI_Send(&median, 1, MPI_LONG, i, 1, MPI_COMM_WORLD);
            }
        }
        else {
            long source_median = world_rank - world_rank % pow_level;
            MPI_Recv(&median, 1, MPI_LONG, source_median, 1, MPI_COMM_WORLD, &status);
        }

        // Split along median
        split(local, left, right, median, size_left, size_right);

        if(world_rank % pow_level < pow_level/2) {
            source = world_rank;
            dest = world_rank + pow_level/2;
        }
        else {
            source = world_rank - pow_level/2;
            dest = world_rank;
        }

        // Send array sizes and arrays
        if(world_rank % pow_level < pow_level/2) {
            MPI_Send(&size_right, 1, MPI_LONG, dest, 1, MPI_COMM_WORLD); 
            MPI_Recv(&size_left, 1, MPI_LONG, dest, 1, MPI_COMM_WORLD, &status);
            MPI_Send(&right[0], size_right, MPI_LONG, dest, 1, MPI_COMM_WORLD); 
            temp.resize(size_left);
            MPI_Recv(&temp[0], size_left, MPI_LONG, dest, 2, MPI_COMM_WORLD, &status);
        }
        else {
            MPI_Recv(&size_right, 1, MPI_LONG, source, 1, MPI_COMM_WORLD, &status);
            MPI_Send(&size_left, 1, MPI_LONG, source, 1, MPI_COMM_WORLD); 
            temp.resize(size_right);
            MPI_Recv(&temp[0], size_right, MPI_LONG, source, 1, MPI_COMM_WORLD, &status);
            MPI_Send(&left[0], size_left, MPI_LONG, source, 2, MPI_COMM_WORLD); 
        }

        local.clear();

        if(world_rank % pow_level < pow_level/2)
            merge(local, left, temp);
        else 
            merge(local, temp, right);

        left.clear();
        right.clear();
        temp.clear();
        size_local = local.size();
    }

    clock_t end = clock();

    // Gather all the sorted lists to the main process and print to output file
    if(world_rank == 0) {
        
        ofstream myfile;
        myfile.open (filename);
        for(int i = 0; i < local.size(); i++) {
            myfile << local[i] << " ";
        }
        for(int i = 1; i < world_size; i++) {
            MPI_Recv(&size_local, 1, MPI_LONG, i, 1, MPI_COMM_WORLD, &status);
            temp.resize(size_local);
            MPI_Recv(&temp[0], size_local, MPI_LONG, i, 1, MPI_COMM_WORLD, &status);
            for(int j = 0; j < size_local; j++) {
                myfile << temp[j] << " ";
            }
        }
        myfile.close();
    }
    else {
        MPI_Send(&size_local, 1, MPI_LONG, 0, 1, MPI_COMM_WORLD);
        MPI_Send(&local[0], size_local, MPI_LONG, 0, 1, MPI_COMM_WORLD);
    }

    cout<<(double)(end - begin) / CLOCKS_PER_SEC;

    // // Finalize the MPI environment.
    MPI_Finalize();
}