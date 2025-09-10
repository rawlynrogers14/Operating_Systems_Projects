// Name  : Rawlyn Rogers 
// Class : COP 4600, Operating Systems
// Program 3 (Condition Varables to protect a limited size resource)
// Date Stated:     Nov 26 2024
// Date Completed:  Nov 29 2024
// Main File
/*
main function: 
    Allocates shared variables and flags that will be shared 
    among Producer and Consumer threads, Initializes conditional variables. 
    Creates a Producer and consumer thread that share these arguments as pointers. 
    The Main function(parent process) waits until both threads have completed. 
    Deallocates shared variables and flags, releases conditional variables.

Producer thread function: 
    Declares local variables and flags. Loops infinitely until exit flag is raised.
    Waits for shared buffer to be empty via a signal, then determines if there is a job in progress.

    If there is no job the producer thread prompts the user for an input, then creates an input buffer
    that stores a set amount of char for user input. The user input is stored in this buffer. 
    The input buffer is compared to see if it matches the phrase “exit”. If so, the input buffer 
    is deallocated and an exit flag is raised, a signal is sent to the consumer thread, and the 
    producer thread terminates. Specification about the input is printed on the screen and the job flag 
    is raised.

    If there is a job the producer reads char at time into the shared input buffer until it is full. Once 
    the shared buffer is full the buffersISFull flag is raised, and a signal is sent to the consumer thread
    to let it know the shared buffer is full. Once all char are read from the input buffer to the shared
    buffer the job flag is lowered and the input buffer is deallocated. 

Consumer thread function: 
    Declares local variables and flags. Loops infinitely until exit flag is called. When a exit flag is 
    called the thread terminates. Waits for shared buffer to be full via a signal. Reads the entire shared 
    buffer one char at a time and prints them onto the screen in order. Once all char in the shared buffer 
    are read the buffersISFull flag is lowered, and a signal is sent to the Producer thread notifying it 
    that the buffer is empty.

*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

//Define Sizes
#define input_buffer_size 50        //Size of input buffer
#define shared_buffer_size 15       //Size of Shared buffer between therads

//Declare mutex and condition variable
pthread_mutex_t protect;
pthread_cond_t buffer_full;
pthread_cond_t buffer_empty;

//create arg structure that will be shared between Produce and Cosumer threads
struct thread_arg{
    
    int *shared_buffer_index;    
    char *shared_buffer;
    bool *exit;
    bool *job;
    bool *bufferISfull;
    
};

//Declare Prototypes
void *Producer_thread(void *arg);
void *Consumer_thread(void *arg);

int main(void){

    //Declare Variables and Flags
    pthread_t th[2];   
    int shared_buffer_index = 0;    
    char shared_buffer[shared_buffer_size];    
    bool exit = false;
    bool job = false;
    bool bufferISfull = false;
    

    //Initialize mutex and condition variable
    pthread_mutex_init(&protect, NULL);
    pthread_cond_init(&buffer_full, NULL);
    pthread_cond_init(&buffer_empty, NULL);  
    

    //Allocates shared Memroy among thread  
    struct thread_arg *shared_Arg = malloc(sizeof(struct thread_arg));        
    shared_Arg->shared_buffer_index = &shared_buffer_index;        
    shared_Arg->shared_buffer = shared_buffer;
    shared_Arg->exit = &exit;
    shared_Arg->job = &job;
    shared_Arg->bufferISfull = &bufferISfull;      
                

    //Create Producer and Consumer threads
    if((pthread_create(&th[0], NULL, &Producer_thread, shared_Arg) != 0  )){ printf("ERROR: Producer Thread could not be created\n"); }        
    if((pthread_create(&th[1], NULL, &Consumer_thread, shared_Arg) != 0  )){ printf("ERROR: Consumer Thread could not be created\n"); }

    //Wait for both threads to finish
    if (pthread_join(th[0], NULL) != 0){ printf("ERROR: Producer Thread could not be joined\n\n"); }        
    if (pthread_join(th[1], NULL) != 0){printf("ERROR: Consumer Thread could not be joined\n\n"); }        
    
    
    //Destroy mutex and condition variable and free allocated resources 
    pthread_mutex_destroy(&protect);
    pthread_cond_destroy(&buffer_full);
    pthread_cond_destroy(&buffer_empty);
    free(shared_Arg);
    
    //Parent Procceess waits until both threads have finished.
    printf("Parent: done\n");
    return 0;
}

//Define Functions
void *Producer_thread(void *arg){
    
    //Declare Variables and Flags
    struct thread_arg* args = (struct thread_arg*) arg;    
    int input_buffer_index = 0;
    char *input_buffer;
    char ch;    
    
    while(1){        
        
        pthread_mutex_lock(&protect);
        while(*args->bufferISfull){

            //Waits for signale From Consumer thread           
            pthread_cond_wait(&buffer_empty, &protect);

        }

        if(!*args->job){
            
            //Allocates and Fills the Input Buffer        
            printf("Enter input (type 'exit' to quit): ");
            input_buffer_index = 0;
            input_buffer = (char *)malloc(input_buffer_size + 1* sizeof(char));

            while((ch = getchar()) != '\n'){
                
                if(input_buffer_index < input_buffer_size){
                    
                    //printf("input_buffer_index: %d\n", input_buffer_index);
                    input_buffer[input_buffer_index] = ch;
                    input_buffer_index++;

                }

            }     
            input_buffer[input_buffer_index] = '\0';           
                        
            //check for exit flag
            if(strcmp(input_buffer, "exit") == 0){
                
                free(input_buffer);
                *args->bufferISfull = true;                              
                *args->exit = true;
                pthread_mutex_unlock(&protect);
                pthread_cond_signal(&buffer_full);                
                return NULL;

            }

            //Prints Input Information
            printf("Input: %s\n", input_buffer);
            printf("Count: %d characters\n", input_buffer_index);
            input_buffer_index = 0;
            *args->job = true;
  
            
        }

        if(*args->job){            
            
            //Resets Buffer Index
            *args->shared_buffer_index = 0;

            //stores ch in form input buffer to shared buffer
            while(input_buffer[input_buffer_index] != '\0' && *args->shared_buffer_index < shared_buffer_size){
                
                args->shared_buffer[*args->shared_buffer_index] = input_buffer[input_buffer_index];
                printf("Produced: %c\n", args->shared_buffer[*args->shared_buffer_index]);
                input_buffer_index++;
                *args->shared_buffer_index = *args->shared_buffer_index + 1;

            }         
                        
            //checks to see if job is finished
            if(*args->shared_buffer_index < shared_buffer_size){
                
                *args->job = false;
                free(input_buffer);
                printf("Produced: done\n");

            }
            
        } 

        *args->bufferISfull = true;               
        pthread_mutex_unlock(&protect);
        
        //Signals consumer thread
        pthread_cond_signal(&buffer_full);
        
    }
    
}
void *Consumer_thread(void *arg){

    //Declare Variables and Flags
    struct thread_arg* args = (struct thread_arg*) arg;
    
    while(1){
        
        pthread_mutex_lock(&protect);
        while(!*args->bufferISfull){

            //Waits for signale From Producer thread              
            pthread_cond_wait(&buffer_full, &protect);
            
        }
        
        //checks for exit Flag
        if(*args->exit){return NULL;}

        //Reads context of shared buffer and prints
        int i = 0;
        while(i < *args->shared_buffer_index){                       
            
            printf("Consumed: %c\n", args->shared_buffer[i]);
            i++;   
        
        }
        
        if(!*args->job){printf("Consumed: done\n");}        

        *args->bufferISfull = false;
        pthread_mutex_unlock(&protect);
        
        //Signals Producer thread
        pthread_cond_signal(&buffer_empty);
       

    }

}