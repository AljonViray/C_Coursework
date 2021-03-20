// Aljon Viray (86285526) and Daniel Mishkanian (44251598)
// HW 6 [SERVER] for ICS 53 - Harris, Winter 2021

/*
- How to compile and run (do this in openlab in the ~/ics_53/hw5 folder)
ssh circinus-11.ics.uci.edu
clear && gcc -pthread server.c -o server && ./server 51515
*/

#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h> 
#define MAXCHARS 200
#define LISTENQ 1024
#define DEBUG 0

typedef struct sockaddr SA;
static int byte_cnt; /* Byte counter */
static sem_t mutex; /* and the mutex that protects it */

typedef struct ClientData {
    int client_ID;
    FILE* file;
    char file_name[MAXCHARS];
    char file_mode;	
} ClientData;
struct ClientData client_data[4] = { {-1, NULL, "", 'x'}, {-1, NULL, "", 'x'}, {-1, NULL, "", 'x'}, {-1, NULL, "", 'x'} };

////////////////////////////////////////////////////////////////////////


static void init_commands (void)
{
    sem_init(&mutex, 0, 1);
    byte_cnt = 0;
}


void commands (int connfd)
{
    // Template variables
    int n_bytes;
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    // Our variables
    char in_msg[MAXCHARS] = {0}; 
    char* out_msg;
    char cmd1[MAXCHARS] = {0}; 
    char cmd2[MAXCHARS] = {0};
    int client_index;


    // Call a thread to handle this client
    pthread_once(&once, init_commands);
    while( (n_bytes = read(connfd, in_msg, MAXCHARS)) != 0 ) {
        sem_wait(&mutex); // semaphore begin

        // Read in message from client, whose ID is "fd"
        if (DEBUG) printf("thread %d received %d bytes on fd %d\n", (int)pthread_self(), n_bytes, connfd);

        // Create new Client ID in Client Data, or get existing client
        // 1st pass to find existing client
        int i = 0; 
        int client_exists = 0;
        for (i = 0; i < 4; i++) {
            if (client_data[i].client_ID == connfd) {
                client_index = i;
                client_exists = 1;
                break;
            }
        }
        // 2nd pass to create new client if needed
        if (!client_exists) {
            for (i = 0; i < 4; i++) {
                if (client_data[i].client_ID == -1) {
                    client_index = i;
                    client_data[i].client_ID = connfd;
                    break;
                }
            }
        }

        sscanf(in_msg, "%s %s", cmd1, cmd2);
        if (strcmp(cmd1, "quit") != 0) printf("%s\n", in_msg); // required print
        out_msg = "";

        /////////////////////////////////////////////////////

        // Command #1 - openRead
        if (strcmp(cmd1, "openRead") == 0) 
        {
            int i;
            int file_is_open = 0;

            if (client_data[client_index].file != NULL) {
                file_is_open = 1;
                out_msg = "A file is already open for reading";
            }

            else {
                for (i = 0; i < 4; i++) {
                    if (i != client_index && client_data[i].file != NULL && strcmp(client_data[i].file_name, cmd2) == 0 && client_data[i].file_mode == 'a') {
                        file_is_open = 1;
                        out_msg = "The file is open by another client.";
                    }
                }
            }

            if (!file_is_open) {
                // We can always assume the file exists for appending
                client_data[client_index].file = fopen(cmd2, "r"); // r = read mode
                strcpy(client_data[client_index].file_name, cmd2);
                client_data[client_index].file_mode = 'r';
            }
        }


        // Command #2 - openAppend
        if (strcmp(cmd1, "openAppend") == 0) 
        {
            int i;
            int file_is_open = 0;

            if (client_data[client_index].file != NULL) {
                file_is_open = 1;
                out_msg = "A file is already open for appending";
            }

            else {
                for (i = 0; i < 4; i++) {
                    if (i != client_index && client_data[i].file != NULL && strcmp(client_data[i].file_name, cmd2) == 0) {
                        file_is_open = 1;
                        out_msg = "The file is open by another client.";
                    }
                }
            }

            if (!file_is_open) {
                // We can always assume the file exists for appending
                client_data[client_index].file = fopen(cmd2, "a"); // a = append mode
                strcpy(client_data[client_index].file_name, cmd2);
                client_data[client_index].file_mode = 'a';
            }
        }


        // Command #3 - read
        else if (strcmp(cmd1, "read") == 0) 
        {
            if (client_data[client_index].file == NULL) 
                out_msg = "File not open";

            else {
                // Read chars until end of file or until reached n_bytes threshold
                char output[MAXCHARS] = {0};
                int i = 0; int c;
                int n = atoi(cmd2);
                while (i < n) {
                    c = fgetc(client_data[client_index].file);
                    if (c == EOF) break;
                    output[i++] = (char)c;
                }
                output[i] = '\0'; // terminate w/ null character 
                out_msg = output;
            }
        }


        // Command #4 - append
        else if (strcmp(cmd1, "append") == 0) 
        {
            if (client_data[client_index].file == NULL) 
                out_msg = "File not open";
            else {
                // Append chars starting at end of file
                fprintf(client_data[client_index].file, cmd2); // NOTE: file will only update after being closed, including in VS CODE
                if (DEBUG) printf("appended %s to %s\n", cmd2, client_data[client_index].file_name);
            }
        }


        // Command #5 - close
        else if (strcmp(cmd1, "close") == 0) 
        {
            // We can always assume file exists and is open, for us to close it.
            if (strcmp(cmd2, client_data[client_index].file_name) == 0) {
                if (DEBUG) printf("closing %s\n", client_data[client_index].file_name);

                // Reset the contents of this client_data, except for clientID
                fclose(client_data[client_index].file);
                client_data[client_index].file = NULL;
                memset(client_data[client_index].file_name, 0, MAXCHARS); 
                client_data[client_index].file_mode = 'x';
            }
        }


        // Command #6 - quit
        else if (strcmp(cmd1, "quit") == 0) 
        {
            // Deletes client from list of active clients. Server stays active.
            client_data[client_index].client_ID = -1;
            
            if (client_data[client_index].file != NULL) {
                fclose(client_data[client_index].file);
                client_data[client_index].file = NULL;
            }

            memset(client_data[client_index].file_name, 0, MAXCHARS); 
            client_data[client_index].file_mode = 'x';
        }

        /////////////////////////////////////////

        // Print contents of client_data after doing stuff
        if (DEBUG)
        {
            printf("-----------------\n");  
            for (i = 0; i < 4; i++) {
                printf("Client #%d (FD=%d)\t->file='%s',\tmode='%c'\n", i, client_data[i].client_ID, client_data[i].file_name, client_data[i].file_mode);
            }
            printf("-----------------\n");  
        }

        // Send message to client
        if (DEBUG) printf("[TO CLIENT #%d (FD=%d)] = %s\n\n", client_index, connfd, out_msg);
        write(connfd, out_msg, MAXCHARS);

        // Reset char arrays
        memset(in_msg, 0, MAXCHARS); memset(cmd1, 0, MAXCHARS); memset(cmd2, 0, MAXCHARS);

        sem_post(&mutex); // semaphore end
    }
}


int open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);

    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) continue;
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(listenfd);
    }

    freeaddrinfo(listp);
    if (!p) return -1;
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
        return -1;
    }
    return listenfd;
}


void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    commands(connfd);
    close(connfd);
    return NULL;
}


//////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) 
{
    // Step 1: Socket and server setup
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    listenfd = open_listenfd(argv[1]);
    printf("server started\n");

    while (1) 
    {
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (SA *) &clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0; 
}