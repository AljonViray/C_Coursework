// Aljon Viray (86285526) and Daniel Mishkanian (44251598)
// HW 6 [CLIENT] for ICS 53 - Harris, Winter 2021

/*
- How to compile and run (do this in openlab in the ~/ics_53/hw5 folder)
clear && gcc -pthread client.c -o client && ./client circinus-11.ics.uci.edu 51515
*/

#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define MAXCHARS 200
#define DEBUG 0


////////////////////////////////////////////////////////////////////////


// Takes in hostname and port #, tries to connect to a server there
int open_clientfd(char* hostname, char* port) 
{
    int clientfd;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* …using numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    getaddrinfo(hostname, port, &hints, &listp);

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) 
    {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */
    
        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* Success */

        close(clientfd); /* Connect failed, try another */
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) return -1;      /* All connects failed */
    else return clientfd;   /* The last connect succeeded */
}


////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv) 
{ 
    int clientfd;
    int is_quitting = 0;
    char in_msg[MAXCHARS]; 
    char out_msg[MAXCHARS]; 
    char cmd1[MAXCHARS]; 
    char cmd2[MAXCHARS];

    // Initialize client socket/connection to server
    char* host = argv[1]; char* port = argv[2];
    clientfd = open_clientfd(host, port);

    // Write inputs to server and receieve messages from server
    while (1) 
    {
        // Reset char arrays
        memset(in_msg, 0, MAXCHARS); memset(out_msg, 0, MAXCHARS); memset(cmd1, 0, MAXCHARS); memset(cmd2, 0, MAXCHARS);

        // Take in input, Tokenize/Parse it
        printf("> ");
        fgets(out_msg, MAXCHARS, stdin);
        out_msg[strlen(out_msg)-1] = '\0'; // remove newline from out_msg
        sscanf(out_msg, "%s %s", cmd1, cmd2);

        // Quit (this client only)
        if (strlen(cmd2) == 0 && strcmp(cmd1, "quit") == 0) is_quitting = 1;

        // Do nothing on every correct case for syntax, otherwise invalid syntax
        else if (strlen(cmd2) > 0) {
            if      (strcmp(cmd1, "openRead"));
            else if (strcmp(cmd1, "openAppend"));
            else if (strcmp(cmd1, "read"));
            else if (strcmp(cmd1, "append"));
            else if (strcmp(cmd1, "close"));
            else {
                printf("Invalid syntax\n");
                continue;
            }
        }

        else {
            printf("Invalid syntax\n");
            continue;
        }

        // Send message to server
        write(clientfd, out_msg, strlen(out_msg));
        if (is_quitting) break;

        // Print out server's reply
        read(clientfd, in_msg, MAXCHARS);
        if (strlen(in_msg) > 0) printf("%s\n", in_msg); // required print, from server
    }

    close(clientfd);
    return 0;
}