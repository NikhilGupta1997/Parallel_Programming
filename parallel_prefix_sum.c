#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <string.h>
#include <pthread.h>
#include <omp.h>
#include <openssl/md5.h>

long long *sums;
long long SIZE;
int NUMTHREADS = 16;
pthread_t threads[256];

struct data {
	long long start;
	long long finish;
	long long d;
}thread_data[256];

long long mylog2(long long num) {
	long long count = 0;
	while(num != 0) {
		num = num>>1;
		count++;
	}
	return count;
}

void *upthread(void * th_data) {
  	struct data *t_data = (struct data*)th_data;
  	long long start, end, i, d;
  	start = t_data->start;
  	end = t_data->finish;
  	d = t_data->d;
  	for (i=start*(1<<(d+1)); i+(1<<(d+1))-1<= end*(1<<(d+1)); i = i + (1<<(d+1))){
      	sums[(long long)(i + (1<<(d+1)) -1)] = sums[(long long)(i + (1<<(d)) -1)] + sums[(long long)(i + (1<<(d+1)) -1)];
  	}
}

void *downthread(void * th_data) {
  	struct data *t_data = (struct data*)th_data;
  	long long start, end, i, d;
  	start = t_data->start;
  	end = t_data->finish;
  	d = t_data->d;
  	for(i=start*(1<<(d+1)); i+(1<<(d+1))-1<= end*(1<<(d+1)); i = i + (1<<(d+1))) {
		long long temp = sums[(long long)(i + (1<<(d)) -1)];
		sums[(long long)(i + (1<<(d)) -1)] = sums[(long long)(i + (1<<(d+1)) -1)];
		sums[(long long)(i + (1<<(d+1)) -1)] = temp + sums[(long long)(i + (1<<(d+1)) -1)];
	}
}

void upsweep(long long arr[]) {
	for(long d = 0; d <= mylog2(SIZE) - 1; d++) {
		long long parses = (long long)(SIZE/(1<<(d+1)));
		for (int i=0; i<NUMTHREADS; i++){
			thread_data[i].start = parses*i/NUMTHREADS;
			thread_data[i].finish = parses*(i+1)/NUMTHREADS;
			thread_data[i].d = d;
	    	int rc  = pthread_create(&threads[i], NULL, upthread, (void*) &thread_data[i]);
	    	if (rc){
	      		printf("ERROR; return code from pthread_create() is %d\n", rc);
	      		exit(-1);
	    	}
	  	}
	  	for (int i=0; i<NUMTHREADS; i++) {
		    pthread_join(threads[i], NULL);
		}
	}
}

void downsweep(long long arr[]) {
	arr[SIZE-1] = 0;
	for(long long d = mylog2(SIZE)-1; d >= 0; d--) {
		long long parses = (long long)(SIZE/(1<<(d+1)));
		for (int i=0; i<NUMTHREADS; i++){
			thread_data[i].start = parses*i/NUMTHREADS;
			thread_data[i].finish = parses*(i+1)/NUMTHREADS;
			thread_data[i].d = d;
	    	int rc  = pthread_create(&threads[i], NULL, downthread, (void*) &thread_data[i]);
	    	if (rc){
	      		printf("ERROR; return code from pthread_create() is %d\n", rc);
	      		exit(-1);
	    	}
	  	}
	  	for (int i=0; i<NUMTHREADS; i++) {
		    pthread_join(threads[i], NULL);
		}
	}
}

int main(int argc, char *argv[]) {
	FILE *inptr, *outptr;
	NUMTHREADS = atoi(argv[1]);

	inptr = fopen("input.txt", "r");
	fscanf(inptr,"%lld",&SIZE);

	printf("%s\n","Allocating Space");
	sums = malloc (sizeof(long long)*SIZE);

	for (long long i=0; i<SIZE; i++) {
    	fscanf(inptr,"%lld",&sums[i]);
	}

	long long last = sums[SIZE-1];

	printf("%s\n","Starting");
	double start, end; 
	start = omp_get_wtime();

	upsweep(sums);
	downsweep(sums);

	end = omp_get_wtime();
	double totalTime = (double)(end - start); 

	outptr = fopen("output.txt", "w+");
	for(int i = 1; i < SIZE; i++) 
		fprintf(outptr, "%lld ", sums[i]);
	fprintf(outptr, "%lld", sums[SIZE-1]+last);
	fclose(outptr);

 	unsigned char c[MD5_DIGEST_LENGTH];
	int bytes;
    unsigned char data[1024];
	MD5_CTX mdContext;
	MD5_Init (&mdContext);
	FILE *readptr;
	readptr = fopen("output.txt", "r");
    while ((bytes = fread (data, 1, 1024, readptr)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
    outptr = fopen("output.txt", "w+");
    fprintf(outptr, "Threads: %d\n", NUMTHREADS);
    fprintf(outptr, "Time:    %f\n", totalTime);
    fprintf(outptr, "Md5-sum: ");
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
		fprintf(outptr, "%02x", c[i]);
	}
	fclose(outptr);
	printf("%s\n","Finished");
	return 1;
}
