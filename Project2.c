//Group Members 
//Rawlyn Rogers, Xi Zhou, John Rojas		
//Class: COP 4600, Operating Systems
//Program 2 (Vido Compression)
//Date Started :	Nov 06 2024	 
//Date Completed :	NOV 19 2024
//main File
/*
	Program Improvments
	Uses thread to speed up video compression program
	Program only uses 20 threads at a time (including the main thread);	
*/


#include <dirent.h> 
#include <stdio.h> 
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1048576 // 1MB

//Create Varables
#define MaxThreads 19  		//Amount Addtional threads that can be created outside of main thread
pthread_mutex_t mutex;
FILE *f_out;

//Declare Prototype
void* Proccess_File(void *arg);

//create Structure for thread Arguments
struct thread_arg{

    int index;
	int thread;
	int *total_inP;
	int *total_outP;
	char **fileList;
	char *argv1;
	pthread_t threadID;	   

};


int cmp(const void *a, const void *b) {
	return strcmp(*(char **) a, *(char **) b);
}

int main(int argc, char **argv) {
	// time computation header
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	// end of time computation header

	// do not modify the main function before this point!

	assert(argc == 2);

	DIR *d;
	struct dirent *dir;
	char **files = NULL;
	int nfiles = 0;

	d = opendir(argv[1]);
	if(d == NULL) {
		printf("An error has occurred\n");
		return 0;
	}

	// create sorted list of PPM files
	while ((dir = readdir(d)) != NULL) {
		files = realloc(files, (nfiles+1)*sizeof(char *));
		assert(files != NULL);

		int len = strlen(dir->d_name);
		if(dir->d_name[len-4] == '.' && dir->d_name[len-3] == 'p' && dir->d_name[len-2] == 'p' && dir->d_name[len-1] == 'm') {
			files[nfiles] = strdup(dir->d_name);
			assert(files[nfiles] != NULL);

			nfiles++;
		}
	}
	closedir(d);
	qsort(files, nfiles, sizeof(char *), cmp);

	// create a single zipped package with all PPM files in lexicographical order
	int total_in = 0, total_out = 0;
	f_out = fopen("video.vzip", "w");
	assert(f_out != NULL);

	pthread_mutex_init(&mutex, NULL);
	int i = 0;
	int j = 0;
		
	for(i=0; i < nfiles; ) {

		//create threads		
		pthread_t th[MaxThreads];		
		int t_created = 0;
		for(j=0; j < MaxThreads && i < nfiles; i++, j++){			
			
			//Create Argument Varables
			struct thread_arg* args = malloc(sizeof(struct thread_arg));
			args->total_inP = &total_in;
			args->total_outP = &total_out;
			args->fileList = files;
			args->argv1 = argv[1];
			args->index = i;
			args->thread = j;

			if(j != 0 ){args->threadID = th[j-1];}

			if(pthread_create(&th[j], NULL, &Proccess_File, args) != 0){
					printf("failer to create thread %d\n", i);
			}
			
		}
		t_created = j;	
					
					
		if(pthread_join(th[t_created-1], NULL) != 0){
				printf("failer to join thread%d\n", i);
		}
			
				
	}
	fclose(f_out);
	pthread_mutex_destroy(&mutex);

	printf("Compression rate: %.2lf%%\n", 100.0*(total_in-total_out)/total_in);

	// release list of files
	for(i=0; i < nfiles; i++)
		free(files[i]);
	free(files);

	// do not modify the main function after this point!

	// time computation footer
	clock_gettime(CLOCK_MONOTONIC, &end);
	printf("Time: %.2f seconds\n", ((double)end.tv_sec+1.0e-9*end.tv_nsec)-((double)start.tv_sec+1.0e-9*start.tv_nsec));
	// end of time computation footer

	return 0;
}

void* Proccess_File(void *arg){

	struct thread_arg* args = (struct thread_arg*) arg;		
	char **files = args->fileList;
	char *arg1 = args->argv1;
	int i = args->index;
	int j = args->thread;
	pthread_t threadID = args->threadID;


	int len = strlen(arg1)+strlen(files[i])+2;
	char *full_path = malloc(len*sizeof(char));
	assert(full_path != NULL);
	strcpy(full_path, arg1);
	strcat(full_path, "/");
	strcat(full_path, files[i]);

	unsigned char buffer_in[BUFFER_SIZE];
	unsigned char buffer_out[BUFFER_SIZE];

	// load file
	FILE *f_in = fopen(full_path, "r");
	assert(f_in != NULL);
	int nbytes = fread(buffer_in, sizeof(unsigned char), BUFFER_SIZE, f_in);
	fclose(f_in);
	
	pthread_mutex_lock(&mutex);
	*args->total_inP += nbytes;	
	pthread_mutex_unlock(&mutex);
	

	// zip file
	z_stream strm;
	int ret = deflateInit(&strm, 9);
	assert(ret == Z_OK);
	strm.avail_in = nbytes;
	strm.next_in = buffer_in;
	strm.avail_out = BUFFER_SIZE;		
	strm.next_out = buffer_out;
	

	ret = deflate(&strm, Z_FINISH);
	assert(ret == Z_STREAM_END);
	
	//waits for previous thread before contiuming the funciton
	if(j != 0 ){
		
		if(pthread_join(threadID, NULL) != 0){
			printf("failer to join thread %d\n", i);
		}

	}
		
	// dump zipped file
	int nbytes_zipped = BUFFER_SIZE-strm.avail_out;
	fwrite(&nbytes_zipped, sizeof(int), 1, f_out);
	fwrite(buffer_out, sizeof(unsigned char), nbytes_zipped, f_out);	
	
	pthread_mutex_lock(&mutex);
	*args->total_outP += nbytes_zipped;
	pthread_mutex_unlock(&mutex);
	
	free(full_path);
	free(arg);
	return NULL;
}
