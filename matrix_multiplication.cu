#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <cuda.h>
#include <mpi.h>

// Global Definitions
#define ELEMS 20
#define max 100
#define THREADS_PER_BLOCK 512
#define THREADS_PER_WARP 32

using namespace std;

// Scalar multiplication :: Has many drawbacks
__global__ void scalar_mul(long long *num_row,const long long *ptr,const int *indices, const long long *data , const long long *x, long long *y) {
    int row =  blockDim.x * blockIdx.x + threadIdx.x;
    if(row < *num_row) {
        long long sum_prod = 0.0;
        long long st = ptr[row] ;
        long long end = ptr[row+1];
        for(long long j = st; j < end; j++) {
            sum_prod+= data[j] * x[indices[j]];
        }
        y[row] += sum_prod;
    }
}

// Improved version :: Better use of sequential addressing in warps
__global__ void vector_mul(long long *num_row,const long long *ptr,const int *indices, const long long *data , const long long *x, long long *y) {
    __shared__ long long values[THREADS_PER_BLOCK]; 
    int tid = blockDim.x * blockIdx.x + threadIdx.x;
    int warp = tid / THREADS_PER_WARP;
    int row = warp;
    int warp_tid = tid % 32;

    if(row < *num_row) {
        long long st = ptr[row] ;
        long long end = ptr[row+1] ;
        values[threadIdx.x] = 0.0;
        
        for(long long j = st + warp_tid; j < end; j += 32)
            values[threadIdx.x] += data[j] * x[indices[j]];
        
        if(warp_tid<16) values[threadIdx.x] += values[threadIdx.x+16]; 
        if(warp_tid<8) values[threadIdx.x] += values[threadIdx.x+8];
        if(warp_tid<4) values[threadIdx.x] += values[threadIdx.x+4];
        if(warp_tid<2) values[threadIdx.x] += values[threadIdx.x+2]; 
        if(warp_tid<1) values[threadIdx.x] += values[threadIdx.x+1];    

        if(warp_tid == 0 ) {    
            y[row] += values[threadIdx.x];
        }
    }

}

int main(int argc, char *argv[]) {
	MPI_Init(&argc,&argv);
    
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    long long *ptr;
    vector<int> column;
    vector<long long> data;
    long long *vec;
    long long size;

    // Read input and initialize
    if(my_rank == 0) {
        fstream file;
        file.open(argv[1]);
        char temp[100];
        file >> temp;
        char name[100];
        file >> name;
        file >> temp;
        file >> size;

        ptr = new long long[size+1];
        file >> temp;
        char input[20];

        ptr[0] = 0;
        long long old = 0;
        long long i = 0;
        while(!file.eof()) {
            file >> input;
            string input_str(input);
            if(input_str.compare("B") == 0) 
                break;
            long long row = atoll(input);
            if(row != old) {
            	for(long long j = old+1; j<row; j++)
            		ptr[j] = i;
          		ptr[row] = i;
          		old = row;
            }
            file >> input;
            int col = atoi(input);
            column.push_back(col);
            file >> input;
            long long dat = atoll(input);
            data.push_back(dat);
            i++;	 
        }
        
        ptr[size] = i; 
        long long index;
        vec = new long long[size];
        i = 0;
        while(!file.eof()) {
            file >> input;
            index = atoll(input);
            vec[i]= index;
            i++;
        }
    }    
       
    // Input, Output done
    // Time to distribute the data
    long long new_size,dat_size;
    vector<long long> process_vec;
    vector<int> process_col;
    vector<long long> process_dat;
    long long *process_ptr ;
    long long *row_st = new long long[world_size];
    long long *row_end = new long long[world_size];
    long long *val_start = new long long[world_size];
    long long *val_end = new long long[world_size];
    long long *new_size1 = new long long[world_size];
    long long *dat_size1 = new long long[world_size];
    
    // Distribute Data
    if(my_rank == 0) {         
        long long total = ptr[size];
        long long st1 = 0, end1 = total/world_size;
        int k = 0;
        for(long long i = 0;i<size;i++) {
            if(ptr[i]>=st1) {
                row_st[k] = i;
                val_start[k] = ptr[i];
                while(i<size && ptr[i]<end1) {
                    i++;
                }
                row_end[k] = i;
                val_end[k] = ptr[i];
                new_size1[k] = row_end[k] - row_st[k];
                dat_size1[k] = val_end[k] - val_start[k];
                k++;
                i--;
                st1 = (k*total)/world_size;
                end1 = ((k+1)*total)/world_size;
            }
        } 
        
        new_size = new_size1[0];
        dat_size = dat_size1[0];
        
        process_vec.resize(size);
        memcpy(&process_vec[0],vec,size*sizeof(long long));
        process_col.resize(dat_size);
        process_dat.resize(dat_size);
        memcpy(&process_col[0],&column[0],dat_size*sizeof(int));
        memcpy(&process_dat[0],&data[0],dat_size*sizeof(long long));
        process_ptr = new long long[new_size1[0]+1];
        memcpy(process_ptr,ptr,(new_size1[0]+1)*sizeof(long long));    
        
        for(int j = 1;j<world_size;j++) {
            cout<<"Sending to "<<j<<", "<<row_st[j]<<","<<val_start[j]<<endl;
            MPI_Send(&new_size1[j],1,MPI_LONG_LONG,j,2,MPI_COMM_WORLD);
            MPI_Send(&dat_size1[j],1,MPI_LONG_LONG,j,3,MPI_COMM_WORLD);
            MPI_Send(&ptr[row_st[j]],(new_size1[j]+1),MPI_LONG_LONG,j,4,MPI_COMM_WORLD);
            MPI_Send(&size,1,MPI_LONG_LONG,j,5,MPI_COMM_WORLD);
            MPI_Send(&vec[0],size,MPI_LONG_LONG,j,6,MPI_COMM_WORLD);
            MPI_Send(&column[val_start[j]],dat_size1[j],MPI_INT,j,7,MPI_COMM_WORLD);
            MPI_Send(&data[val_start[j]],dat_size1[j],MPI_LONG_LONG,j,8,MPI_COMM_WORLD);
        }
    }
    // Receive Data
    else {
        cout<<"Rank rec "<<my_rank<<endl;
        MPI_Recv(&new_size,1,MPI_LONG_LONG,0,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&dat_size,1,MPI_LONG_LONG,0,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        process_ptr = new long long[new_size+1];
        process_col.resize(dat_size);
        process_dat.resize(dat_size);
        MPI_Recv(process_ptr,new_size+1,MPI_LONG_LONG,0,4,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&size,1,MPI_LONG_LONG,0,5,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        process_vec.resize(size);
        MPI_Recv(&process_vec[0],size,MPI_LONG_LONG,0,6,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&process_col[0],dat_size,MPI_INT,0,7,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        MPI_Recv(&process_dat[0],dat_size,MPI_LONG_LONG,0,8,MPI_COMM_WORLD,MPI_STATUS_IGNORE);     
        cout<<"Rank received "<<my_rank<<endl;        
    }

    // Distribution done
    long long old_ptr = process_ptr[0];
    for(long long j =0;j<new_size+1;j++) {
        process_ptr[j] = process_ptr[j] - old_ptr;
    }    
    cout<<"Rank here is "<<my_rank<<" :: "<<new_size<<","<<dat_size<<","<<endl;     
    long long *num_row;
    long long *host_ans = new long long[new_size];
    long long *zero_ans = new long long[new_size];
    memset(zero_ans,0.0,new_size*sizeof(long long));
    
    int  *gpu_col;
    long long *gpu_ptr;
    long long *gpu_vec,*gpu_dat , *gpu_ans;

    // Allocate CUDA memory
    cudaMalloc((void **)&gpu_vec,size*sizeof(long long));
    cudaMalloc((void **)&gpu_ans,new_size*sizeof(long long));
    cudaMalloc((void **)&gpu_col,dat_size*sizeof(int));
    cudaMalloc((void **)&gpu_dat,dat_size*sizeof(long long));
    cudaMalloc((void **)&gpu_ptr,(new_size+1)*sizeof(long long));
    cudaMalloc((void **)&num_row,sizeof(long long));

    // Initialize CUDA memory
    cudaMemcpy(gpu_col,&process_col[0],dat_size*sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(gpu_dat,&process_dat[0],dat_size*sizeof(long long), cudaMemcpyHostToDevice);
    cudaMemcpy(num_row,&new_size,sizeof(long long),cudaMemcpyHostToDevice);
    cudaMemcpy(gpu_vec,&process_vec[0],size*sizeof(long long), cudaMemcpyHostToDevice);
    cudaMemcpy(gpu_ptr,&process_ptr[0],(new_size+1)*sizeof(long long), cudaMemcpyHostToDevice);
    cudaMemcpy(gpu_ans,zero_ans,(new_size)*sizeof(long long), cudaMemcpyHostToDevice);
    
    int num_block;

    if(new_size%16==0)
        num_block = new_size/16;
    else
        num_block = new_size/16 + 1;

    // Run CUDA multiplication
    vector_mul<<<num_block,THREADS_PER_BLOCK>>>(num_row,gpu_ptr,gpu_col,gpu_dat,gpu_vec,gpu_ans);
    cudaMemcpy(host_ans,gpu_ans,new_size*sizeof(long long), cudaMemcpyDeviceToHost);
       
    MPI_Status status;
    vector<long long> tempo;
    long long size_ans;

    // Receive the segmented output and print to a file
    if(my_rank == 0) {
        ofstream myfile;
        myfile.open(argv[2]);
        for(long long i = 0; i < new_size; i++) {
            myfile << host_ans[i] << "\n";
        }
        for(int i = 1; i < world_size; i++) {
            MPI_Recv(&size_ans, 1, MPI_LONG_LONG, i, 1, MPI_COMM_WORLD, &status);
            tempo.resize(size_ans);
            MPI_Recv(&tempo[0], size_ans, MPI_LONG_LONG, i, 1, MPI_COMM_WORLD, &status);
            for(long long j = 0; j < size_ans; j++) {
                myfile << tempo[j] << "\n";
            }
        }
        myfile.close();
    }
    else {
        MPI_Send(&new_size, 1, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
        MPI_Send(&host_ans[0], new_size, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
    }

    // Free CUDA memory
    cudaFree(num_row);
    cudaFree(gpu_col);
    cudaFree(gpu_dat);
    cudaFree(gpu_ans);
    cudaFree(gpu_ptr);
    cudaFree(gpu_vec);
    MPI_Finalize();

}