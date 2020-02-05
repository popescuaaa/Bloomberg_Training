#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define error_message_file "Error when tring to open/create file!\n"

typedef double complex cplx;
FILE *input;
FILE *output; 
char *input_file_name;
char *output_file_name;
int number_of_elements;
int number_of_threads;
int global_step = 1;
pthread_mutex_t mutex;

// setup for parallel computing 
typedef struct {
	cplx *buf;
	cplx *out;
	int step;
} _fft_args;


void getArgs(int argc, char **argv);
// thread function
void *thread_fft_function(void * args);

// Recursive function for fft which stands also as
// the main thread generator for the whole process
// In this function we can see 2 parts:
// - the thread creation part and 
// - the number processing part,
//   which is not changed from that on Rosetta 
// I speculate the fact that the number of threads and the
// number of elements are both 2's powers and I create
// threads until I reach the specified number in cmd line args
// for eq: 4 -> the main call will create 2 threads and those 2
// will create another 2; and now I have 4 threads that enter 
// in recursivity and then the results are combines tree like
// until the main function, in this case, _fft


void _fft(cplx buf[], cplx out[], int step);

int main(int argc, char** argv){
	getArgs(argc, argv);
	int i;

	input = fopen(input_file_name, "r");
	if(input == NULL){
		printf(error_message_file);   
		exit(1);             
	}

	output = fopen(output_file_name, "w");
	if(output == NULL){
		printf(error_message_file);   
		exit(1);             
	}

	if (fscanf(input, "%d", &number_of_elements) == EOF) {
		printf(error_message_file);   
		exit(1); 
	}
	cplx *buf = (cplx *) malloc(number_of_elements * sizeof(cplx));

	for (i = 0; i < number_of_elements; i++) {
			double value;
			if(fscanf(input, "%lf", &value) == EOF){
				printf(error_message_file);   
				exit(1);             
			}
			buf[i] = CMPLX(value, 0);
	}

	cplx *out =  (cplx *) malloc(number_of_elements * sizeof(cplx));

	for (int i = 0; i < number_of_elements; i++) out[i] = buf[i];
	
	_fft(buf, out, 1);
	

	fprintf(output, "%d\n", number_of_elements);
	for (i = 0; i < number_of_elements; i++) {
		fprintf(output, "%0.6lf %0.6lf\n", creal(buf[i]), cimag(buf[i]));
	}

	fclose(input);
	fclose(output);
	free(buf);
	free(out);
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

void *thread_fft_function(void * args) {
	_fft_args thread_args = *(_fft_args *) args;
	_fft(thread_args.buf, thread_args.out, thread_args.step);

	return NULL;
}

void _fft(cplx buf[], cplx out[], int step){
	if (step < number_of_elements) {
		// is a hybrid mix between recursivity and 
		// parallelism
		if (step < number_of_threads) {
			pthread_t tid_odd, tid_even;
			_fft_args main_args_odd;
			_fft_args main_args_even;

			main_args_odd.buf = out;
			main_args_odd.out = buf;
			main_args_odd.step = 2 * step;

			main_args_even.buf = out + step;
			main_args_even.out = buf + step;
			main_args_even.step = 2 * step;

			pthread_create(&(tid_odd), NULL, thread_fft_function , &(main_args_odd));
			pthread_create(&(tid_even), NULL, thread_fft_function , &(main_args_even));

			pthread_join(tid_odd, NULL);
			pthread_join(tid_even, NULL);
		} else {
			_fft(out, buf, step * 2);
			_fft(out + step, buf + step, step * 2);
		}
 
		for (int i = 0; i < number_of_elements; i += 2 * step) {
			cplx t = cexp(-I * M_PI * i / number_of_elements) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + number_of_elements)/2] = out[i] - t;
		}
	}
}