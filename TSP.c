#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <string.h>
#include <pthread.h>
#include <omp.h>
#include <math.h>
#include <limits.h>

// Global Variables
int pop_size = 10000;
int parent_size = 800;
int iter = 50000;
int iter_limit = 15000;

// City Structure
struct city {
	char id;
	float x, y;
}cities[36];

// Chromosome Structure
struct chrome {
	float length;
	char chromosome[36];
};

// Calculates the distance between two cities
float dist(int a, int b) {
	float x1, x2, y1, y2;
	x1 = cities[a-1].x;
	y1 = cities[a-1].y;
	x2 = cities[b-1].x;
	y2 = cities[b-1].y;
	return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

// Maps the int index of a city to a character
char map_char(int index) {
	if(index <= 26) 
		return (char)(index+64);
	else 
		return (char)(index-26+48);
}

// Maps the character representation of a city to its corresponding id
int map_int(char index) {
	if((int)(index) > 64) 
		return (int)(index) - 64;
	else
		return (int)(index) - 48 + 26;
}

// Calculates the path length of a chosen chromosome
float path_length(int size, char chromosome[]) {
	float length = 0.0;
	int index1, index2;
	for(int i = 0; i < size; i++) {
		index1 = map_int(chromosome[i]);
		if(i != size-1)
			index2 = map_int(chromosome[i+1]);
		else 
			index2 = map_int(chromosome[0]);
		length += dist(index1, index2);
	}
	return length;
}

// Value of Chromosome to differentiate cyclic equality
long value(int size, char chrome[]) {
	long val = 0;
	for(int i = 0; i < size; i++) {
		val *= 10;
		val += (int)chrome[i];
	}
	return val;
}

// Swap the postions of two chromosomes in the array
void swap(struct chrome population[], int a, int b, int size){
	struct chrome temp;
	temp.length = population[b].length;
	for(int k = 0; k < size; k++) {
		temp.chromosome[k] = population[b].chromosome[k];
	}
	population[b].length = population[a].length;
	for(int k = 0; k < size; k++) {
		population[b].chromosome[k] = population[a].chromosome[k];
	}
	population[a].length = temp.length;
	for(int k = 0; k < size; k++) {
		population[a].chromosome[k] = temp.chromosome[k];
	}
}

// QuickSort to sort the chromosomes
int sort(int size, struct chrome population[], int piv, int high) {
	if(piv >= high){
		return 0;
	}

	float pivot = population[piv].length;
	int head = piv+1;
    int tail = high;

	while(head < tail){
		if(population[head].length > pivot || (population[head].length == pivot && value(size, population[head].chromosome) > value(size, population[piv].chromosome))){
			swap(population, head, tail, size);
			tail = tail-1;
		}
		else{
			head = head+1;
		}
	}

	if(population[head].length > pivot){
		swap(population, piv, head-1, size);
		sort(size, population, piv, head-2);
		sort(size, population, head, high);
	}
	else{
		swap(population, piv, head, size);
		sort(size, population, piv, head-1);
		sort(size, population, head+1, high);
	}
	return 0;
}

// Used to create a random chromosome to create a population
void random_chromosome(int size, char pop[]) {
	int index[36] = {0};
	int id, count = 0;
	while(count < size) {
		id = rand()%size;
		if(index[id] != 1) {
			pop[count] = map_char(id+1);
			count++;
			index[id] = 1;
		}
	}
}

// The Cycle Crossover
void CX(int size, char chrome1[], char chrome2[], char target[]) {
	int index[36] = {0};
	int id = 0;
	char start = chrome1[0], curr;
	index[(map_int(start)-1)] = 1;
	while(1) {
		curr = chrome2[id];
		if(index[(map_int(curr)-1)] == 1)
			break;
		index[(map_int(curr)-1)] = 1;
		for(int i = 0; i < size; i++) {
			if(chrome1[i] == curr) {
				id = i;
				break;
			}
		}
	}
	for(int i = 0; i < size; i++) {
		if(index[(map_int(chrome1[i])-1)] == 1)
			target[i] = chrome1[i];
		else 
			target[i] = chrome2[i];
	}
}

// The Partially Mapped Crossover
void PMX(int size, char chrome1[], char chrome2[], char target[]) {
	char temp1[36], temp2[36];
	int index[36] = {0};
	int index1[36] = {0};
	int index2[36] = {0};
	int length = rand() % (size-4) + 1;
	int start = rand() % (size-length+1); 
	for(int i = start; i < start+length; i++) {
		temp1[i] = chrome2[i];
		temp2[i] = chrome1[i];
		index[(map_int(chrome1[i])-1)] += 1;
		index[(map_int(chrome2[i])-1)] += 1;
		index1[i] = 1;
		index2[i] = 1;
	}
	for(int i = 0; i < size; i++) {
		if(index1[i] == 1) {}
		else if(index[map_int(chrome1[i])-1] == 0) {
			index1[i] = 1;
			temp1[i] = chrome1[i];
		}
		else {
			int flag = 1;
			char temp;
			char curr = chrome1[i];
			while(flag) {
				for(int m = 0; m < size; m++) {
					if (chrome2[m] == curr) {
						temp = chrome1[m];
						break;
					}
				}
				if(index[map_int(temp)-1] == 1)
					flag = 0;
				curr = temp;
			}
			index1[i] = 1;
			temp1[i] = curr;
			for(int m = 0; m < size; m++) {
				if(chrome2[m] == curr) {
					temp2[m] = chrome1[i];
					index2[m] = 1;
				}
			}
		}
		if(index2[i] == 1) {}
		else if(index[map_int(chrome2[i])-1] == 0) {
			index2[i] = 1;
			temp2[i] = chrome2[i];
		}
		else {
			int flag = 1;
			char temp;
			char curr = chrome2[i];
			while(flag) {
				for(int m = 0; m < size; m++) {
					if (chrome1[m] == curr) {
						temp = chrome2[m];
						break;
					}
				}
				if(index[map_int(temp)-1] == 1)
					flag = 0;
				curr = temp;
			}
			index2[i] = 1;
			temp2[i] = curr;
			for(int m = 0; m < size; m++) {
				if(chrome1[m] == curr) {
					temp1[m] = chrome2[i];
					index1[m] = 1;
				}
			}
		}
	}
	float len1 = path_length(size, temp1), len2 = path_length(size, temp2);
	if(len1 < len2) {
		for(int i = 0; i < size; i++) {
			target[i] = temp1[i];
		}
	}
	else {
		for(int i = 0; i < size; i++) {
			target[i] = temp2[i];
		}
	}
}

// The Greedy Crossover
void GX(int size, char chrome1[], char chrome2[], char target[]) {
	int index[36] = {0};
	target[0] = chrome1[0];
	int id = map_int(target[0]) - 1;
	index[id] = 1;
	for(int i = 1; i < size; i++) {
		char a = chrome1[0], b = chrome2[0];
		for(int j = 0; j < size-1; j++) {
			if(chrome1[j] == target[i-1]) {
				a = chrome1[j+1];
				break;
			}
		}
		for(int j = 0; j < size-1; j++) {
			if(chrome2[j] == target[i-1]) {
				b = chrome2[j+1];
				break;
			}
		}
		if(index[map_int(a)-1] == 1) {
			if(index[map_int(b)-1] == 1) {
				do {
					id = rand() % size;
				} while (index[id] == 1);
				target[i] = map_char(id+1);
				index[id] = 1;
			}
			else {
				target[i] = b;
				index[map_int(b)-1] = 1;
			}
		}
		else {
			if(index[map_int(b)-1] == 1) {
				target[i] = a;
				index[map_int(a)-1] = 1;
			}
			else {
				float len1 = dist(map_int(target[i-1]), map_int(a)), len2 = dist(map_int(target[i-1]), map_int(b));
				if(len1 < len2) {
					target[i] = a;
					index[map_int(a)-1] = 1;
				}
				else {
					target[i] = b;
					index[map_int(b)-1] = 1;
				}
			}
		}
	}
}

// Perform Mutation of a chromosome
void mutation(int size, char chrome[]) {
	int index1 = rand() % size;
	int index2 = rand() % size;
	char temp;
	temp = chrome[index2];
	chrome[index2] = chrome[index1];
	chrome[index1] = temp;
}

// Generate the Children from the parents
void generate_children(int size, struct chrome parents[]) {
	// Parallelize the child genration
	#pragma omp parallel for
	for(int i = 0; i < parent_size; i += 2) {
		int index1 = rand() % parent_size;
		int index2 = rand() % parent_size;
		int cross = rand() % 1000;
		// Choose crossover with 0.8 probability
		if(cross < 800 && omp_get_thread_num()%2 == 1) {
			GX(size, parents[index1].chromosome, parents[index2].chromosome, parents[i + parent_size].chromosome);
			PMX(size, parents[index1].chromosome, parents[index2].chromosome, parents[i+1 + parent_size].chromosome);
		}
		else if(cross < 800 && omp_get_thread_num()%2 == 0) {
			CX(size, parents[index1].chromosome, parents[index2].chromosome, parents[i + parent_size].chromosome);
			GX(size, parents[index1].chromosome, parents[index2].chromosome, parents[i+1 + parent_size].chromosome);
		}
		// Child = Parent
		else {
			for(int k = 0; k < size; k++) {
				parents[i + parent_size].chromosome[k] = parents[index1].chromosome[k];
			}
			for(int k = 0; k < size; k++) {
				parents[i+1 + parent_size].chromosome[k] = parents[index2].chromosome[k];
			}
		}

		// Perform mutation on child 1 with probability 0.1
		int mut = rand() % 1000;
		if(mut < 100) {
			mutation(size, parents[i + parent_size].chromosome);
		}
		// Perform mutation on child 2 with probability 0.1
		mut = rand() % 1000;
		if(mut < 100) {
			mutation(size, parents[i+1 + parent_size].chromosome);
		}
		parents[i + parent_size].length = path_length(size, parents[i + parent_size].chromosome);
		parents[i+1 + parent_size].length = path_length(size, parents[i+1 + parent_size].chromosome);
	}
}

// Check to see if two chromosomes are the same (Excluding Cyclic Equality)
int equate(int size, struct chrome first, struct chrome second) {
	if(first.length != second.length)
		return 0;
	for(int i = 0; i < size; i++) {
		if(first.chromosome[i] != second.chromosome[i])
			return 0;
	}
	return 1;
}

int main(int argc, char *argv[]) {
	// Generate Random seed
	srand(time(NULL));

	// Input Variables
	int size = 0;
	char name[50];
	struct chrome population[pop_size], parents[2*parent_size], children[2*parent_size];

	// File read variables
	FILE *file = fopen( argv[1], "r" );
	char *line = NULL;
	size_t len = 0;
	
    
	// Read information from the specified input file
    if ( file == 0 )
        printf( "Could not open file\n" );

    int count = 0;
    while ((getline(&line, &len, file)) != -1) {
    	count++;
    	char siz[2];
    	char ch;

    	// Read file name and convert it into txt
    	if(count == 1) {
    		int c = 0, iter = 0;
    		do {
    			ch = line[iter];
    			iter++;
    		} while(ch != ' ');
    		do {
    			ch = line[iter];
    			iter++;
    			name[c] = ch;
    			c++;
    		} while(line[iter] != '\n');
    	}

    	// Read the Dimension
    	if(count == 2) {
    		int c = 0, iter = 0;
    		do {
    			ch = line[iter];
    			iter++;
    		} while(ch != ' ');
    		do {
    			ch = line[iter];
    			iter++;
    			siz[c] = ch;
    			c++;
    		} while(line[iter] != '\n');
    		size = atoi(siz);	
    	}

    	// Read each city and its coordinates
    	if(count >= 4 && line[1] != 'E') {
    		char val1[2] = {0}, val2[15] = {0}, val3[15] = {0};
    		int c1 = 0, c2 = 0, c3 = 0;
    		int iter = 0;
    		// printf("line = %s", line);
    		while(line[iter] == ' ')
    			iter++;
    		do {
    			ch = line[iter];
    			iter++;
    			val1[c1] = ch;
    			c1++;
    		} while(ch != ' ');
    		while(line[iter] == ' ')
    			iter++;
    		do {
    			ch = line[iter];
    			iter++;
    			val2[c2] = ch;
    			c2++;
    		} while(ch != ' ');
    		while(line[iter] == ' ')
    			iter++;
    		do {
    			ch = line[iter];
    			iter++;
    			val3[c3] = ch;
    			c3++;
    		} while(line[iter] != '\n');

    		cities[count-4].id = map_char(atoi(val1));
			cities[count-4].x = atof(val2);
			cities[count-4].y = atof(val3);
    	}
    }

    // Get the number of threads
	int num_threads = atoi(argv[2]);
	omp_set_num_threads(num_threads);

	// Print some important information to user
	printf("\nFile Name = %s", name);
	printf("\nDimension = %d", size);
	printf("\nNumber of Threads = %d\n", num_threads);
	
	for(int i = 0; i < size; i++) {
		printf("%c ", cities[i].id);
		printf("%f ", cities[i].x);
		printf("%f\n", cities[i].y);
	}

	// Change global variables based on input values
	pop_size *= (float)(size)/(float)36;
	parent_size *= (float)(size)/(float)36;
	iter_limit *= (float)(size)/(float)36;

	// Generate a Random Population
	for(int i = 0; i < pop_size; i++) {
		char temp[36];
		random_chromosome(size, temp);
		float length = path_length(size, temp);
		population[i].length = length;
		for(int j = 0; j < size; j++) {
			population[i].chromosome[j] = temp[j];
		} 
	}

	// Sort the population based on fitness function
	sort(size, population, 0, pop_size-1);

	// Choose Best 500 parents
	for(int i = 0; i < parent_size; i++) {
		parents[i].length = population[i].length;
		for(int k = 0; k < size; k++) {
			parents[i].chromosome[k] = population[i].chromosome[k];
		}
	}

	int counter = 0;
	float min = INT_MAX;
	int limit_count = 0;
	for(int p = 0; p < iter; p++) {

		// Parent Iteration
		counter++;
		limit_count++;
		generate_children(size, parents);
		sort(size, parents, 0, 2*parent_size-1);

		// Remove Duplicates
		for(int i = 0, j = 0; i < parent_size; i++, j++) {
			while(equate(size, parents[j], parents[j+1])){
				j++;
			}
			children[i].length = parents[j].length;
			for(int k = 0; k < size; k++) {
				children[i].chromosome[k] = parents[j].chromosome[k];
			}
		}

		// Update the best found path
		if(children[0].length < min) {
			min = children[0].length;
			limit_count = 0;
		}

		// Child Iteration
		counter++;
		limit_count++;
		if(limit_count > iter_limit) 
			break;
		generate_children(size, children);
		sort(size, children, 0, 2*parent_size-1);

		// Remove Duplicates
		for(int i = 0, j = 0; i < parent_size; i++, j++) {
			while(equate(size, children[j], children[j+1]))
				j++;
			parents[i].length = children[j].length;
			for(int k = 0; k < size; k++) {
				parents[i].chromosome[k] = children[j].chromosome[k];
			}
		}

		// Update the best found path
		if(parents[0].length < min) {
			min = parents[0].length;
			limit_count = 0;
		}
	}

	// Print Results
	printf("%s %f  %s\n","Best match =" ,parents[0].length, parents[0].chromosome);
	printf("Number of iterations = %d\n", counter);

	// Write to an output file
	fclose( file );

	int newSize = strlen(name)  + strlen(".txt") + 1; 
	char *newName = (char *)malloc(newSize);
	strcpy(newName,name);
   	strcat(newName,".txt");
	FILE *f = fopen( newName, "w" );
	fprintf(f, "DIMENSION : %d\n", size);
	fprintf(f, "TOUR_LENGTH : %f\n", parents[0].length);
	fprintf(f, "TOUR_SELECTION\n");
	for(int i = 0; i < size; i++) {
		fprintf(f, "%d\n", map_int(parents[0].chromosome[i]));
	}
	fprintf(f, "%d\n", -1);
	fclose( f );

	return 0;
}
