// Name  : Rawlyn Rogers 
// Class : COP 4600, Operating Systems
// Program 1 (Rush Shell Program)
// Date Stated:     Sep 26 2024
// Date Completed:  Oct 20 2024
// Main File
/*
    
    Shell program description:

	Shell program Infinitely  loops, continuously prompting the user “ rush>”  for input. 
    The input of the user is then stored and formatted so that the shell can understand it. 
    Inputs can have as many whitespaces as possible and tabs between all separate elements of the command. 
    This shell dose not take any command line arguments, a error will be issued and the shell will exit. 
    
    Basic format:   rush > cmd0 arg0… argN >RedirectionPath & cmd1 arg0 … argN >RedirectionPath …. & cmdN
        
        The shell has 3 built in commands:

            Exit -	Exits the program when the  user types “exit”. The program waits for all parallel commands 
            to finish, deallocated all dynamic memory, and finally issues an exit(0) command, which will exit the 
            shell. This command does not take any arguments.

            Change Directory – Uses chdir() system call to change to the desired directory. Will issue an error if 
            there is an issue. This command takes only one argument , the file path of the directory the user wishes 
            to change to.

            Path -  Stores and update current file path for commands, Starts with default file path “/bin”. When 
            command is issued all stored paths are deleted. And new paths are then stored as current file paths. 
            If a commands file path is not currently stored, shell will not run that command. Paths do not affect 
            built in commands; they will always function. This command takes zero to several arguments.

        When an executable command is issued, a new process is created to execute the command. The user will not 
        be prompted until all processes are finished executing or are terminated. All executable commands can be 
        run in parallel. All commands that want to be run in parallel must be separated by “&”.  Blank commands do 
        not issue an error, all executable commands have redirection capabilities. Redirection requests can be 
        executed with all commands running in parallel. Multiple redirections are not supported in the same command, 
        an error will be issued.

        All errors are written to “stander error” and then ffush() so that they are printed in the correct order. 
        There is only one built in error message “An error has occurred\n”.

        Program Map:
	        Main{		
		        Loop	{ 
                    Get user input
			        Built in commands{}
			        Executable Commands {
				        Checks paths{
					    Checks for Redirection
					    No Redirection {}
					    Redirection {}
				    }
                    Wait for parallel Commands
                    Exit _flag 				
			    }			
            }
           
	    }

*/

//directives
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>                 
#include <fcntl.h>

//Constants 
const int BufferSize = 255;                                                 //Buffer size
const int PathNumber = 200;                                                 //Number of paths that can be stored.

//Funtion Prototypes
void get_input(char *buffer, size_t buffsize);                              // Recives input
void format_input(char *buffer);                                            // Formats input
void wait_for_children();                                                   // waits for all children to complet there tasks
void free_ptr_list(char **ptr_list, int list_size);                         // frees ptr list type

int collect_args(char **ptr_list,  char *string_arg, int starting_index);   // Collects arguments from string and places it into ptr_list, returns list size
int count_redirection(char *string_statment);                               // Counts the number of '>' string statment has, ruturns count



int main(int argc, const char *argv[]){

    //Checks for command line arguments    
    if(argv[1] != NULL){

        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);

    }

    //creat varables
    //creates a buffer and path list
    //checks if it was successful
	size_t buffsize = BufferSize;	
	char *buffer = malloc(sizeof(char) * BufferSize);
    char **path_list = malloc(sizeof(char * ) *PathNumber);
    char **args_list = malloc(sizeof(char * ) *PathNumber);      
    int path_index = 1;       
	if(buffer == NULL || path_list == NULL || args_list == NULL ){
		
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
	}   

    //add default path /bin
    path_list[0] = malloc(10* sizeof(char)); 
    strcpy(path_list[0], "/bin");    
    bool exit_flag = false;

    //Loops util exit command
    while(1){
    
        //get and format inputs
        get_input(buffer, buffsize);        
        format_input(buffer);        

        //Identify statements sepearted by &
        char *current = buffer, *statement = NULL, *cmd = NULL;
        while((statement = strsep(&current, "&")) != NULL){                
                
            
            //checks if command is blank
            if(statement[0] =='\0'){break;}
            if(statement[0] == ' ' ){
                if(current == NULL){break;}                
                printf("rush> \n");
                continue;
            }

            //get command from statement
            cmd = strsep(&statement, " ");

                
            //Built in Commands             
            if(strcmp(cmd, "exit") == 0){ 
                
                //Exit
                //Checks for argumuments if none are present activate exit flag.    

                char *temp = NULL;
                if((temp = strsep(&statement, " ")) != NULL ){
                    char error_message[30] = "An error has occurred\n";
                     write(STDERR_FILENO, error_message, strlen(error_message));                    
                }
                else{exit_flag = true;}

            }else if(strcmp(cmd, "cd") == 0){
                
                // Change Directory
                // Gets path for argument and Changes directory to that path                        

                char *path = strsep(&statement, "&");
                if(chdir(path) != 0){
                    
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    
                }
                

            }else if(strcmp(cmd, "path") == 0){               
                
                // Path
                // Clears all old paths and replaces them with new paths    
                
               
                //clear old paths
                free_ptr_list(path_list, path_index);               
                path_index = 0;
                
                //Collect Arguments and places them into path list            
                char *arg = NULL;                 
                while((arg = strsep(&statement, " ")) != NULL){                   
                                      
                    path_list[path_index] = malloc((strlen(arg) + 1 )* sizeof(char));                                       
                    strcpy(path_list[path_index], arg);
                    path_index ++;                
                }              
  

            }else{
                
                //Executable Commands
                        
                //Create Child Process 
                pid_t p = fork();
                if(p < 0) {

                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);;
                
                }else if( p == 0){

                    
                    //Creat Path Variables and cheak for valid paths
                    char current_path[BufferSize];
                    bool path_flag = false;
                    int args_index;                  
                    int i = 0;

                    for(i = 0; i < path_index; i++ ){                            
                            
                        strcpy(current_path, path_list[i]);
                        current_path[strlen(current_path) + 1] = '\0';
                        current_path[strlen(current_path) ] = '/';                            
                        if(access(strcat(current_path, cmd), X_OK) == 0 ){
                            path_flag = true;
                            break;
                        }

                    }
                    
                    if(path_flag == false){
                        
                        //no valid path found
                        char error_message[30] = "An error has occurred\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(0); 

                    }else{

                        //count number of redirection
                        int numb_redir = count_redirection(statement);          
                
                        if(numb_redir > 1){

                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));                
                              
                        }else if(numb_redir == 0){
                            
                            //no Redireciton


                            //Collect Arguments
                            args_list[0] = malloc((strlen(current_path) + 1 )* sizeof(char));
                            strcpy(args_list[0], current_path);
                            args_index = collect_args(args_list, statement, 1); 


                            //check for warrnings
                            int i = 0;                           
                            for(i=0 ;(i <= args_index && args_list[i] != NULL) && ((strcmp(cmd, "ls") == 0) || (strcmp(cmd, "cat") == 0)); i++){
                                   
                                if(access(args_list[i], F_OK) != 0){                                    
                                
                                char error_message[30];
                                if(strcmp(cmd, "cat") == 0){                                
                                    sprintf(error_message, "%s: %s: No such file or directory\n", cmd, args_list[i]);  


                                }else{

                                    sprintf(error_message, "%s: cannot access '%s': No such file or directory\n", cmd, args_list[i]);

                                }
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                free_ptr_list(args_list, args_index);
                                exit(0);

                                }                               
                                
                            }


                            //executes command, reports error if somthing whent wrong
                            if(execv(args_list[0], args_list) == -1){

                                char error_message[30] = "An error has occurred\n";
                                write(STDERR_FILENO, error_message, strlen(error_message));

                            }   
                            fflush(stdout);   


                        }else{
                            
                            // Redireciton


                            //Create varbles
                            //Get and format redirection path
                            char *arg_part = strsep(&statement, ">");
                            arg_part[strlen(arg_part) - 1] = '\0';                            
                            char *redir_path = strsep(&statement, "&");                         
                                                                                   

                            //Collect Arguments
                            args_list[0] = malloc((strlen(current_path) + 1 )* sizeof(char));
                            strcpy(args_list[0], current_path);
                            args_index = collect_args(args_list, arg_part, 1);   

                            //check for warrnings
                            int i = 0;                           
                            for(i=0 ;(i <= args_index && args_list[i] != NULL) && ((strcmp(cmd, "ls") == 0) || (strcmp(cmd, "cat") == 0)); i++){
                                   
                                if(access(args_list[i], F_OK) != 0){                                    
                                
                                char error_message[30];
                                if(strcmp(cmd, "cat") == 0){                                
                                    sprintf(error_message, "%s: %s: No such file or directory\n", cmd, args_list[i]);  


                                }else{

                                    sprintf(error_message, "%s: cannot access '%s': No such file or directory\n", cmd, args_list[i]);

                                }
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                free_ptr_list(args_list, args_index);
                                exit(0);

                                }                                
                                
                            }

                            //Sends output to file
                            //opens file and reports errors
                            int output_file = open(redir_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                            if(output_file < 0){                                                    
                                
                                char error_message[30] = "An error has occurred\n";
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                free_ptr_list(args_list, args_index);                                ;
                                exit(1);                            
                            }
                            //Send output to file   
                            if(dup2(output_file, STDOUT_FILENO) < 0){
                                
                                char error_message[30] = "An error has occurred\n";
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                free_ptr_list(args_list, args_index);
                                exit(1);

                            }
                            //executes command, reports error if somthing whent wrong
                            if(execv(args_list[0], args_list) == -1){

                                char error_message[30] = "An error has occurred\n";
                                write(STDERR_FILENO, error_message, strlen(error_message));                                                               
                                
                            }
                            fflush(stdout);
                            
                            //closes file
                            close(output_file);

                        }
                        

                    }
                        
                    //free memory and close child proccess
                    free_ptr_list(args_list, args_index);
                    exit(0);
                        
                }


            }
            

        }
        //if exit_flag activated free memory and exit
        wait_for_children();
        if(exit_flag == true){
            
            free(buffer);
            free(args_list);
            free_ptr_list(path_list, path_index);
            free(path_list);
            exit(0);

        }


    }


}


//Function Definitions

//recives input
//gets input from user and places it into buffer
void get_input(char *buffer, size_t buffsize){

    	
    printf("rush> ");
    fflush(stdout);
    size_t read = getline(&buffer, &buffsize, stdin);    
    if(read == -1){
        
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        
    }
               

}

//removes newlines, spaces, and tabs from input.
//formats user input so program can read it
void format_input(char *buffer){
    
    //creates varables
    int i = 0;
    int j = 0;
    char temp[BufferSize];
    
    while(buffer[i] == ' ' || buffer[i] == '\t'){i++;}

    for(;buffer[i] != '\n';){
        
 
        if(buffer[i] != ' ' && buffer[i] != '\t' && buffer[i] != '&' && buffer[i] != '>' ){ //general characters
            
            temp[j] = buffer[i];
            j++;
            i++;

        }
        else if(buffer[i] == ' ' || buffer[i] == '\t' ){            // white spaces
            
            temp[j] = ' ';
            j++;
            i++;
            while(buffer[i] == ' ' || buffer[i] == '\t'){i++;}     

        }
        else if(buffer[i] == '>'){                                  //redirecion
            
            temp[j] = '>';
            j++;
            i++;
            while(buffer[i] == ' ' || buffer[i] == '\t'){i++;}           

        }
        else if(buffer[i] == '&'){
            
            if(j == 0 ){                                            // & is first char
                
                temp[j] = ' ';
                temp[j + 1] = '&';
                j = j + 2;                              
            }            
            else if(temp[j - 1] == '&'){                            // _&&_
                
                temp[j] = ' ';
                temp[j + 1] = '&';
                j = j + 2;              
            
            }
            else if(temp[j - 1] != '&' ){
                                   
                            
                if(temp[j - 1] == ' '){temp[j - 1] = '&';}          // _ &_
                else{                                               // _a&_
                                                           
                    temp[j] = '&';
                    j++;

                }                        
                
            }            
            i++;

            while(buffer[i] == ' ' || buffer[i] == '\t' ){i++;}

        }  


    }

    //Add \0 to end of string
    if(j >= 2){
        
        if(temp[j-1] == ' '){
            
            if(temp[j - 2] != '&'){
                
                j--;
            }
            
        }
        else{
            
            if(temp[j - 1] == '&'){
                
                temp[j] = ' ';
                j++;
            }  

        }    


    }
    temp[j] = '\0';
  
    //copy temp storage of sting into the buffer.
    strcpy(buffer, temp);

} 

//Waits for all child proccess to Complete
//loops wait call until all child prcessess have finished 
void wait_for_children(){

    while(1){      
                    
        pid_t cpid = NULL;
        cpid = wait(NULL);
                       
            if(cpid < 0){            
            break;
            } 

        }  

}

// frees ptr list type
//loops through list and free allocated memory
void free_ptr_list(char **ptr_list, int list_size){

    int i = 0;
    for(i = 0; i < list_size; i++ ){
        
        free(ptr_list[i]);

    }

}

// Collects arguments from string and places it into ptr_list
int collect_args(char **ptr_list,  char *string_arg, int starting_index){

    //Creates varables And Collect argurments
    char *arg = NULL;                            
    int args_index = starting_index;    
                          
    while((arg = strsep(&string_arg, " ")) != NULL){                            
    
        if(arg[0] == '\0'){break;}
        ptr_list[args_index] = malloc((strlen(arg) + 1 )* sizeof(char));
        strcpy(ptr_list[args_index], arg);
        args_index++;

    }    
    ptr_list[args_index] == NULL;      
    return args_index;


}
//counts the number of '>' string statment has
//return the number of char as a int       
int  count_redirection(char *string_statment){  
    
    int i = 0;
    int numb_redir = 0;                  
                                
    if(string_statment != NULL){                                                

        for(i = 0; i < strlen(string_statment) ; i++ ){
            if(string_statment[i] == '>'){                                
                                                                
                if(numb_redir > 1){break;}
                    numb_redir++;
                                
            }

        }

    }
    return numb_redir;
                        

}               