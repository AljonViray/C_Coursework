// Code by Aljon Viray and Daniel Mishkanian
/*
How to compile and run (do this in openlab in the ~/ics_53/hw2 folder)
clear && gcc hw2.c -o hw2 && ./hw2
clear && gcc hw2.c -o hw2 && valgrind ./hw2
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#define MAXLINE 80
#define MAXARGS 80
#define MAXJOB  5

typedef struct Job {
    int jid;
    int pid;
    int status; // 0 = Foreground, 1 = Running (in Background), 2 = Stopped
    int argc;
    char *argv[MAXARGS];
} Job;

char input[MAXLINE]; // Original input, max size of 80 chars
char buffer[MAXLINE]; // Copy of input
char *argv[MAXARGS]; // Array of pointers to token strings, will not actually use all 80 slots
char *delim, *redirect_in, *redirect_out;
int i = 0, j = 0, argc = 0, numJobs = 0; // 'argc' is the number of arguments

// Stuff for the process to remember what it is
int isBackground = 0; // "Boolean" flags
pid_t fork_pid = -1; // Process id of this process (changes when fork() happens)
struct Job *jobs;


////////////////////////////////////////////////////////////////////


// Turn raw input from commandline into argv list
int parseLine (char *buf, char **argv, int *argc, int *isBackground) 
{
    char *delim;
    int temp_argc = 0; // Number of args
    int bg = 0; // "Boolean", Is this a background job?

    buf[strlen(buf)-1] = ' '; // Replace trailing '\n' with space
    while (*buf && (*buf == ' ')) { // Ignore leading spaces
        buf++;
    }

    // Build the argv list
    temp_argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[temp_argc++] = buf;
        *delim = '\0'; // End of line character
        buf = delim + 1;
        while (*buf && (*buf == ' ')) { // Ignore spaces
            buf++;
        }
    }
    argv[temp_argc] = NULL;

    // Check if there is an '&' to signal this is a background process
    if (*argv[temp_argc-1] == '&') {
        bg = 1;
        // Remove the '&' so it doesnt disrupt the argv commands
        temp_argc--;
        argv[temp_argc] = NULL;
    }  

    // "Return" values of bg and argc
    *argc = temp_argc;
    *isBackground = bg;

    return 0;
}



void deleteJob (int indexToDelete) {
    for (i = 0; i < indexToDelete; i++) {
        free(jobs[indexToDelete].argv[i]);
    }
    // Overwrite the Job, move all Jobs after this one to the left of the array
    for (i = indexToDelete; i < numJobs-1; i++) {
        jobs[i] = jobs[i+1];
    }
    numJobs--;
}



void sigtstp_handler(int sig) 
{
    for (i = 0; i < numJobs; i++) {
        if (jobs[i].pid == fork_pid) {
            // kill(jobs[i].pid, SIGTSTP); // Send STOP signal to child/job
            jobs[i].status = 2;
            // printf("Stopped child [%d] w/ sigtstp_handler\n", jobs[i].pid);
        }
    }
}



void sigint_handler(int sig) 
{
    // Terminate shell when no jobs are left to terminate
    if (numJobs <= 0) {
        // printf("Terminating shell because no jobs left..\n");
        exit(getpid()); // Exit whole shell if no children are present
    }

    // Terminate current foreground job only, NOT BACKGROUND JOBS
    int status;
    int child_pid = waitpid(fork_pid, &status, WUNTRACED);
    for (i = 0; i < numJobs; i++) {
        if (jobs[i].pid == child_pid) {
            // kill(jobs[i].pid, SIGINT); // Send Ctrl+C signal to child/job
            deleteJob(i);
            // printf("Killed child [%d]\n", jobs[i].pid);
        }
    }
}



void sigchld_handler () {
    int status; // Stores what waitpid() returns
    int child_pid;

    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // printf("SIGCHLD -> STOPPED = %d, CONTINUED = %d, EXITED = %d\n", WIFSTOPPED(status), WIFCONTINUED(status), WIFEXITED(status));
        if (WIFEXITED(status) == 1) {
            for (i = 0; i < numJobs; i++) {
                if (jobs[i].pid == child_pid) {
                    deleteJob(i);
                }
            }
        }
    }
}



int main () 
{
    jobs = malloc(5 * sizeof(Job));
    signal(SIGTSTP, sigtstp_handler); // catch ctrl+z, stop
    signal(SIGINT,  sigint_handler);  // catch ctrl+c, terminate
    signal(SIGCHLD, sigchld_handler); // catch reaping all children?

    while (1) {
        // Reset token array (by just letting this loop overwrite the old data)
        // Take in input, Tokenize/Parse it
        argc = 0;
        printf("prompt> ");
        fgets(input, MAXLINE, stdin);
        if (input[0] == '\n') {
            continue;
        }
        strcpy(buffer, input); // Make a mutable copy of the original input
        parseLine(buffer, argv, &argc, &isBackground);
        

        // Jobs //
        if (strcmp(argv[0], "jobs") == 0 && numJobs < 5) { // Print all running jobs
            // printf("There should be '%d' jobs.\n", numJobs);
            for (i = 0; i < numJobs; i++) {
                printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);

                // 0 = Foreground, 1 = Running (in Background), 2 = Stopped
                if (jobs[i].status == 0) {
                    printf("Foreground ");
                }
                else if (jobs[i].status == 1) {
                    printf("Running "); // "Background"
                }
                else if (jobs[i].status == 2) {
                    printf("Stopped ");
                }

                // Print argvs
                for (j = 0; j < jobs[i].argc; j++) {
                    printf("%s ", jobs[i].argv[j]);
                }
                if (jobs[i].status == 1) {
                    printf("&");
                }
                printf("\n");
            }
            continue;
        } 


        // FG //
        else if (strcmp(argv[0], "fg") == 0) {
            // 0 = Foreground, 1 = Running (in Background), 2 = Stopped
            // Check the second parameter of fg if it's "%[JID]" or [PID]
            char str[2] = "\0";
            str[0] = argv[1][1];
            
            // Find desired Stopdped OR Background, make it Foreground
            // Gave JID
            if (argv[1][0] == '%') {
                for (i = 0; i < numJobs; i++) {
                    if (jobs[i].jid == atoi(str)) {
                        jobs[i].status = 0;
                        // printf("CHANGED Job: jid = %d, pid = %d, status = %d\n", jobs[i].jid, jobs[i].pid, jobs[i].status);
                        kill(jobs[i].pid, SIGCONT); // Send CONTINUE signal to child/job
                        break;
                    }
                }
            }
            // Gave PID
            else {
                for (i = 0; i < numJobs; i++) {
                    if (jobs[i].pid == atoi(argv[1])) {
                        jobs[i].status = 0;
                        // printf("CHANGED Job: jid = %d, pid = %d, status = %d\n", jobs[i].jid, jobs[i].pid, jobs[i].status);
                        kill(jobs[i].pid, SIGCONT); // Send CONTINUE signal to child/job
                        break;
                    }
                }
            }
            // All built-in commands do not continue to fork()
            continue;
        }


        // BG //
        else if (strcmp(argv[0], "bg") == 0) {
            // 0 = Foreground, 1 = Running (in Background), 2 = Stopped
            // Check the second parameter of bg if it's "%[JID]" or [PID]
            char str[2] = "\0";
            str[0] = argv[1][1];
            
            // Find desired Stopped, make it Background
            // Gave JID
            if (argv[1][0] == '%') {
                for (i = 0; i < numJobs; i++) {
                    if (jobs[i].jid == atoi(str) && jobs[i].status == 2) {
                        jobs[i].status = 1;
                        // printf("CHANGED Job: jid = %d, pid = %d, status = %d\n", jobs[i].jid, jobs[i].pid, jobs[i].status);
                        kill(jobs[i].pid, SIGCONT); // Send CONTINUE signal to job
                        break;
                    }
                }
            }
            // Gave PID
            else {
                for (i = 0; i < numJobs; i++) {
                    if (jobs[i].pid == atoi(argv[1]) && jobs[i].status == 2) {
                        jobs[i].status = 1;
                        // printf("CHANGED Job: jid = %d, pid = %d, status = %d\n", jobs[i].jid, jobs[i].pid, jobs[i].status);
                        kill(jobs[i].pid, SIGCONT); // Send CONTINUE signal to job
                        break;
                    }
                }
            }
            // All built-in commands do not continue to fork()
            continue;
        }


        // Kill //
        else if (strcmp(argv[0], "kill") == 0) {
            if (numJobs > 0) {
                // Gave JID
                if (argv[1][0] == '%') {
                    // Check the second parameter of kill if it's "%[JID]" or [PID]
                    char jid[2] = "\0";
                    jid[0] = argv[1][1];
                    for (i = 0; i < numJobs; i++) {
                        if (jobs[i].jid == atoi(jid)) {
                            deleteJob(i);
                            break;
                        }
                    }
                }

                // Gave PID
                else {
                    for (i = 0; i < numJobs; i++) {
                        if (jobs[i].pid == atoi(argv[1])) {
                            deleteJob(i);
                            break;
                        }
                    }
                }
                // printf("Killed Job [%s]\n", argv[1]);
            }
            continue;
        }


        // Quit //
        else if (strcmp(argv[0], "quit") == 0) { // Quit command
            free(jobs);
            exit(getpid()); // Kills Parent Process  
        } 



        // RETURNS 2 THINGS, [0 in child] and [child PID in parent]
        fork_pid = fork(); 

        // Child, Run Executable
        if (fork_pid == 0) { 
            // Try both of the following
            // execvp() : Linux commands {ls, cat, sort, ./hellp, ./slp}. 
            // execv() : Linux commands {/bin/ls, /bin/cat, /bin/sort, hello, slp}.
            int isCommand = -1; // Invalid by default
            isCommand = execvp(argv[0], argv); 
            isCommand = execv(argv[0], argv); 

            if (isCommand < 0) { // execv returns -1 if not a valid command/executable file
                // printf("%s: Command or File not found.\n", argv[0]);
                // "Delete" the child that was not used, shift all jobs back in array
                deleteJob(numJobs);
                exit(0); // Exit the child process only
            }
        }

        // Parent, Add to Job Array, waitpid()
        else {
            struct Job *job = malloc(sizeof(Job));
            job->jid = numJobs+1;
            job->pid = fork_pid;
            job->status = isBackground;
            job->argc = argc;
            for (i = 0; i < argc; i++) {
                job->argv[i] = malloc(strlen(argv[i]) + 1);
                strcpy(job->argv[i], argv[i]);
            }
            jobs[numJobs] = *job;
            numJobs++;
            free(job);

            // [FOREGROUND] process running, wait (CANNOT INPUT INTO SHELL UNTIL DONE)
            int status; // Stores what waitpid() returns
            if (jobs[0].status == 0) { 
                int child_pid = waitpid(fork_pid, &status, WUNTRACED);
                // printf("FG -> STOPPED = %d, CONTINUED = %d, EXITED = %d\n", WIFSTOPPED(status), WIFCONTINUED(status), WIFEXITED(status));
                
                // Cleanup after child terminates
                if (WIFEXITED(status) == 1) {
                    for (i = 0; i < numJobs; i++) {
                        if (jobs[i].pid == child_pid) {
                            deleteJob(i);
                        }
                    }
                }
            }
        }
    } // end of while loop
    return 0;
} // end of main