#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "line_parser.h"
#include "limits.h"
#include "job_control.h"
#include <signal.h>
#define TRUE 1
#define FALSE 0

int status;
int input_flag = 0;
int output_flag = 0;
FILE* in_file;
FILE* out_file;
struct job** job_list;
struct job* new_job;
struct termios* ter;
pid_t first_child;
pid_t shell_pid;
//int blocking = 1;

void sig_def(){
    signal(SIGQUIT,SIG_DFL);
    signal(SIGCHLD,SIG_DFL);
    // SIGTTIN, SIGTTOU, SIGTSTP
    signal(SIGTTIN,SIG_DFL);
    signal(SIGTTOU,SIG_DFL);
    signal(SIGTSTP,SIG_DFL);
}

void sig_handler(int sig){
    char* sigstr = strsignal(sig);
    printf("%s was ignored\n", sigstr);
}

int execute_command(cmd_line* line){
    if(line -> input_redirect != NULL){
        input_flag = 1;
        fclose(stdin);
        in_file = fopen(line -> input_redirect,"r");
    }
    
    if(line -> output_redirect != NULL){
        output_flag = 1;
        fclose(stdout);
        out_file = fopen(line -> output_redirect,"w");
    }
       
    if(execvp(line -> arguments[0], line -> arguments) == -1){
        _exit(-1);
    }
    
    if(input_flag == 1) {
        fclose(in_file);
        input_flag = 0;
    }
    
    if(output_flag == 1) {
        fclose(out_file);
        output_flag = 0;
    }
    return 0;
  
}

int execute_helper(cmd_line* _cmd_line, int left[2],int right[2],int from_main){
    //cmd_line* next = _cmd_line -> next;
    pid_t child_pid = fork();
    if(child_pid == 0){
        if(from_main) first_child = child_pid;
        sig_def();
        setpgid(0,first_child);
         
        if(right!=NULL){
            fclose(stdout);
            dup(right[1]);
            close(right[1]);
        }
        if(left!=NULL){
            fclose(stdin);
            dup(left[0]);
            close(left[0]);
        }
        execute_command(_cmd_line);
    }
    else{
        if(new_job -> pgid == 0) new_job -> pgid = child_pid;
        setpgid(child_pid,new_job -> pgid);
        /*if(from_main) {
            setpgid(child_pid,first_child);
            new_job->pgid = first_child;
        }*/
        
        //printf("%d\n", new_job -> status);
        if(_cmd_line -> blocking) run_job_in_foreground(job_list, new_job, 0, ter, shell_pid);
        else run_job_in_background(new_job,1);
        
        if(right != NULL) close(right[1]);
        if(left != NULL) close(left[0]);
       
        if(_cmd_line -> next != NULL){
            if(_cmd_line -> next -> next == NULL) execute_helper(_cmd_line -> next, right, NULL,0);
            else {
                int new_right[2];
                if(pipe(new_right)==-1) _exit(-1);
                execute_helper(_cmd_line -> next, right, new_right,0);
            }
        }
    }
    
    return 0;
}

int execute(cmd_line* _cmd_line){
    if(_cmd_line -> next == NULL) execute_helper(_cmd_line,NULL,NULL,1);
    else{
        int first_right[2];
        pipe(first_right);
        execute_helper(_cmd_line,NULL,first_right,1);
    }
    return 0;
}



int main(int argc, char* argv[]){
    
    char buff [PATH_MAX];
    char* _getcwd;
    char* line;
    job_list = malloc(sizeof(job*));
    job_list[0] = NULL;
    
    signal(SIGQUIT,SIG_IGN);
    signal(SIGCHLD,SIG_IGN);
    // SIGTTIN, SIGTTOU, SIGTSTP
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    
    setpgid(0,0);
    
    shell_pid = getpid();
    ter = malloc(sizeof(struct termios));
    tcgetattr(STDIN_FILENO, ter);
    
    while (TRUE){
        //blocking = 1;
        _getcwd = getcwd(buff, (size_t)PATH_MAX);
        if(_getcwd !=NULL)
            printf("%s:",_getcwd);
        
        char in_buff [2048];
        line = fgets(in_buff,2048,stdin);
 
        if(strcmp(line,"\n")==0)
            continue;
        
        cmd_line* _cmd_line = parse_cmd_lines(line);
        
        if(strcmp(_cmd_line -> arguments[0],"jobs")==0){
            print_jobs(job_list);
            continue;
        }
        
        if(strcmp(_cmd_line -> arguments[0],"quit")==0){
            free_cmd_lines(_cmd_line);
            free_job_list(job_list);
            free(job_list);
            free(ter);
            return 0;
        }
        
        if(strcmp(_cmd_line -> arguments[0],"fg")==0){
            //printf("%s\n",line+3);
            job* j = find_job_by_index(job_list[0],atoi(line+3));
            if(j != NULL) run_job_in_foreground (job_list, j,j -> status == SUSPENDED, ter, shell_pid);
            
        }
        
        else if(strcmp(_cmd_line -> arguments[0],"bg")==0){ 
            job* j = find_job_by_index(job_list[0],atoi(line+3));
            if(j != NULL) run_job_in_background(j, 1);
            
        }
        
        else {
            new_job = add_job(job_list,line);
            new_job -> status  = RUNNING;
            execute(_cmd_line);
        }
        
        free_cmd_lines(_cmd_line);
        
        
    }
    
    return 0;
}
