// Aljon Viray (86285526) and Daniel Mishkanian (44251598)
// HW 5 [CLIENT] for ICS 53 - Harris, Winter 2021

/*
- How to compile and run (do this in openlab in the ~/ics_53/hw5 folder)
clear && gcc client.c -o client && ./client circinus-11.ics.uci.edu 36486
*/

#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h> 
#define MAXCHARS 255
#define DEBUG 0


////////////////////////////////////////////////////////////////////////


// Takes in hostname (circinus-11.ics.uci.edu) and port # (36482), tries to connect to a server there
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
    int quit = 0;
    char out_msg[MAXCHARS] = {0}; char in_msg[MAXCHARS];
    char cmd1[MAXCHARS] = {0}; char cmd2[MAXCHARS] = {0}; char cmd3[MAXCHARS] = {0}; char cmd4[MAXCHARS] = {0};
    int year; int month; int day;
    char* year_str; char* month_str; char* day_str;

    // Initialize client socket/connection to server
    char* host; char* port;
    host = argv[1]; port = argv[2];
    clientfd = open_clientfd(host, port);

    // Write char[256] inputs to server and receieve messages from server
    // Note: 1st char/byte of message must be length of message
    while (1) 
    {
        // Reset variables, so prompt with ignore blank inputs
        memset(out_msg, 0, MAXCHARS); memset(in_msg, 0, MAXCHARS); 
        memset(cmd1, 0, MAXCHARS); memset(cmd2, 0, MAXCHARS); memset(cmd3, 0, MAXCHARS); memset(cmd4, 0, MAXCHARS);
        year = 0; month = 0; day = 0;

        // Take in input, Tokenize/Parse it
        printf("> ");
        fgets(out_msg, MAXCHARS, stdin);
        out_msg[strlen(out_msg) - 1] = '\0'; // Remove newline
        sscanf(out_msg, "%s %s %s %s", cmd1, cmd2, cmd3, cmd4);

        if (strcmp(cmd1, "Prices") != 0 && strcmp(cmd1, "MaxProfit") != 0 && strcmp(cmd1, "quit") != 0) {
            printf("Invalid syntax\n");
            continue;
        }

        if (strcmp(cmd1, "MaxProfit") == 0 ) {
            if ((strlen(cmd2) == 0 && strlen(cmd3) == 0) || (strlen(cmd2) > 0 && strlen(cmd3) > 0)) {
                printf("Invalid syntax\n");
                continue;
            }
            if (strcmp(cmd2, "AAPL") != 0 && strcmp(cmd2, "TWTR") != 0) {
                printf("Invalid syntax\n");
                continue;
            }
        }

        if (strcmp(cmd1, "Prices") == 0 ) {
            if ((strlen(cmd2) == 0 && strlen(cmd3) == 0) || (strlen(cmd2) > 0 && strlen(cmd3) == 0) || strlen(cmd4) > 0) {
                printf("Invalid syntax\n");
                continue;
            }
            if (strcmp(cmd2, "AAPL") != 0 && strcmp(cmd2, "TWTR") != 0) {
                printf("Invalid syntax\n");
                continue;
            }

            // Date checking (Strings)
            year_str = strtok(cmd3, "-");
            month_str = strtok(NULL, "-");
            day_str = strtok(NULL, "-");
            if (strlen(year_str) != 4 || strlen(month_str) != 2 || strlen(day_str) != 2) {
                printf("Invalid syntax\n");
                continue;
            }

            year = atoi(year_str);

            // set string to int properly for month
            if (strcmp(month_str, "01") == 0) month = 1;
            if (strcmp(month_str, "02") == 0) month = 2;
            if (strcmp(month_str, "03") == 0) month = 3;
            if (strcmp(month_str, "04") == 0) month = 4;
            if (strcmp(month_str, "05") == 0) month = 5;
            if (strcmp(month_str, "06") == 0) month = 6;
            if (strcmp(month_str, "07") == 0) month = 7;
            if (strcmp(month_str, "08") == 0) month = 8;
            if (strcmp(month_str, "09") == 0) month = 9;
            else month = atoi(month_str);

            // set string to int properly for day
            if (strcmp(day_str, "01") == 0) day = 1;
            if (strcmp(day_str, "02") == 0) day = 2;
            if (strcmp(day_str, "03") == 0) day = 3;
            if (strcmp(day_str, "04") == 0) day = 4;
            if (strcmp(day_str, "05") == 0) day = 5;
            if (strcmp(day_str, "06") == 0) day = 6;
            if (strcmp(day_str, "07") == 0) day = 7;
            if (strcmp(day_str, "08") == 0) day = 8;
            if (strcmp(day_str, "09") == 0) day = 9;
            else day = atoi(day_str);
            if (DEBUG) printf("after: %d, %d, %d\n", year, month, day);


            // Date checking (Integers)
            if(year < 0 || year > 9999)
            {
                printf("Invalid syntax\n");
                continue;
            }
            if (month < 1 || month > 12)
            {
                printf("Invalid syntax\n");
                continue;
            }
            if((month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12) && (day<1 || day>31)) {
                printf("Invalid syntax\n");
                continue;
            }
            if((month==4 || month==6 || month==9 || month==11) && (day<1 && day>30)) {
                printf("Invalid syntax\n");
                continue;
            }
            if(month==2 && (day<1 && day>28)) {
                printf("Invalid syntax\n");
                continue;
            }
            if (month==2 && day==29) { // If it is Feb 29th but it's not a leap year, it's wrong
                if (((year % 4 == 0) && (year % 100!= 0)) || (year%400 == 0)) {}
                else {
                    printf("Invalid syntax\n");
                    continue;
                }
            }
        }
            

        // Prepare to quit if requested
        if (strcmp(cmd1, "quit") == 0) {
            if (strlen(cmd2) > 0)
            {
                printf("Invalid syntax\n");
                continue;
            }
            if (DEBUG) printf("Client quitting, telling server to quit too...\n");
            quit = 1;
        }

        int i = 0;
        for (i = strlen(out_msg); i > 0; i--) {
            out_msg[i] = out_msg[i-1];
        }
        out_msg[0] = strlen(out_msg);

        // Send message to server
        if (DEBUG) {
            char* out_msg_str = out_msg;
            out_msg_str += 1; //skips first char for clean printing
            printf("[TO SERVER] = (len=%d)%s\n", out_msg[0], out_msg_str);
        }
        write(clientfd, out_msg, strlen(out_msg));

        if (quit) break;

        // Print out server's reply
        read(clientfd, in_msg, MAXCHARS);
        printf("%s\n", in_msg);
    }

    close(clientfd);
    return 0;
}