#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <pthread.h>

#define error_message_file "Error when tring to open/create file!\n"
#define min(x, y) (((x) < (y)) ? (x) : (y))

// program setup
FILE * in;
FILE * out;
char *input_file_name, *output_file_name;
int number_of_threads, number_of_elements;
double *function_values;
double complex *function_hat_values;

typedef struct {
	int start_point;
	int end_point;
} Interval;

void getArgs(int argc, char **argv);
Interval* break_intervals(int number_of_elements, int number_of_threads);
void *generate_partition(void *arg);

int main(int argc, char * argv[]) {
	getArgs(argc, argv);

	// multithreading setup
	pthread_t tid[number_of_threads];
	int i;

	in = fopen(input_file_name, "r");
	if(in == NULL){
		printf(error_message_file);   
		exit(1);             
	}

	out = fopen(output_file_name, "w");
	if(out == NULL){
		printf(error_message_file);   
		exit(1);             
	}


	if ( fscanf(in, "%d", &number_of_elements) == EOF) {
		printf(error_message_file);   
		exit(1); 
	}

	function_values = (double *) malloc(number_of_elements * sizeof(double));
	function_hat_values = (complex *) malloc(number_of_elements * sizeof(double complex));

	for (i = 0; i < number_of_elements; i++) {
		if (fscanf(in, "%lf", &function_values[i]) == EOF) {
			printf(error_message_file);   
			exit(1); 
		}
	}

	Interval *intervals = break_intervals(number_of_elements, number_of_threads);

	for(i = 0; i < number_of_threads; i++) {
		pthread_create(&(tid[i]), NULL, generate_partition , &(intervals[i]));
	}

	for(i = 0; i < number_of_threads; i++) {
		pthread_join(tid[i], NULL);
	}
	
	fprintf(out, "%d\n", number_of_elements);
	for (i = 0; i < number_of_elements; i++) {
		fprintf(out, "%0.6lf %0.6lf\n", creal(function_hat_values[i]), cimag(function_hat_values[i]));
	}

	free(function_values);
	free(function_hat_values);
	fclose(in);
	fclose(out);
	
	return 0;
}

void getArgs(int argc, char **argv){
	if(argc < 3) {
		printf("Not enough paramters: ./program input_file_name output_file_name number_of_threads\n");
		exit(1);
	}

	input_file_name = argv[1];
	output_file_name = argv[2];
	number_of_threads = atoi(argv[3]);
}

Interval* break_intervals(int number_of_elements, int number_of_threads){
	double number_of_elements_per_partition = 
		ceil(number_of_elements/number_of_threads);
	
	Interval* intervals = (Interval*) malloc(number_of_threads * sizeof(Interval));
	
	intervals[0].start_point = 0;
	intervals[0].end_point = number_of_elements_per_partition;
	
	int i = 0;
	for(i = 1; i < number_of_threads; i++){
		// starting point for the next will be the ending of the one before
		intervals[i].start_point = intervals[i-1].end_point;
		intervals[i].end_point = min(number_of_elements, (number_of_elements_per_partition+1)*(i+1));
	}

	return intervals;
}

void *generate_partition(void *arg) {
	Interval interval = *(Interval*)arg;
	int i, j;
	for( j = interval.start_point ; j < interval.end_point; j++){
		double real = 0, img = 0;
		for( i = 0; i < number_of_elements; i++){
			real += function_values[i] * cos(2 * M_PI * i * j / number_of_elements);
            img -= function_values[i] * sin(2 * M_PI * i * j / number_of_elements);
		}

		function_hat_values[j] = CMPLX(real, img);
	}

	return NULL;
}