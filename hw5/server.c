// Aljon Viray (86285526) and Daniel Mishkanian (44251598)
// HW 5 [SERVER] for ICS 53 - Harris, Winter 2021

/*
- How to compile and run (do this in openlab in the ~/ics_53/hw5 folder)
ssh circinus-11.ics.uci.edu
clear && gcc server.c -o server && ./server AAPL.csv TWTR.csv 36486
*/

#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h> 
#include <math.h>
#define MAXCHARS 255
#define DEBUG 0

typedef struct Stock {
    char date[MAXCHARS];
    float close;	
} Stock;

struct Stock AAPL_stocks[503]; // Each CSV file contains 503 lines of useful data (1st line is just column names)
struct Stock TWTR_stocks[503];


////////////////////////////////////////////////////////////////////////


void store_AAPL_stocks (char const* filepath)
{
    FILE* file;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    file = fopen(filepath, "r");
    int i = 0;

    getline(&line, &len, file); // extra getline() to ignore first line, which is just column names
    while ((read = getline(&line, &len, file)) != -1) {
        // Store 1st and 5th column (index 0 and 4)
        char new_date[MAXCHARS]; float new_close; // Useful data
        char unused_open[MAXCHARS]; char unused_high[MAXCHARS]; char unused_low[MAXCHARS]; char unused_adjclose[MAXCHARS];  char unused_volume[MAXCHARS];
        
        // Have to use [^,] because comma is not recognized as a char normally
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%f,%[^,],%[^,]", new_date, unused_open, unused_high, unused_low, &new_close, unused_adjclose, unused_volume);
        strcpy(AAPL_stocks[i].date, new_date);
        AAPL_stocks[i].close = new_close;
        i++;
    }

    fclose(file);
    if (line) free(line);
}


void store_TWTR_stocks (char const* filepath)
{
    FILE* file;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    file = fopen(filepath, "r");
    int i = 0;

    getline(&line, &len, file); // extra getline() to ignore first line, which is just column names
    while ((read = getline(&line, &len, file)) != -1) {
        // Store 1st and 5th column (index 0 and 4)
        char new_date[MAXCHARS]; float new_close; // Useful data
        char unused_open[MAXCHARS]; char unused_high[MAXCHARS]; char unused_low[MAXCHARS]; char unused_adjclose[MAXCHARS];  char unused_volume[MAXCHARS];
        
        // Have to use [^,] because comma is not recognized as a char normally
        sscanf(line, "%[^,],%[^,],%[^,],%[^,],%f,%[^,],%[^,]", new_date, unused_open, unused_high, unused_low, &new_close, unused_adjclose, unused_volume);
        strcpy(TWTR_stocks[i].date, new_date);
        TWTR_stocks[i].close = new_close;
        i++;
    }

    fclose(file);
    if (line) free(line);
}


////////////////////////////////////////////////////////////////////////


int main(int argc, char const* argv[]) 
{
    // Step 1: Load in CSV files
    // Note: argv is ALWAYS {apple stock filename, twitter stock filename, port number}
    // Though, apple stock filename and twitter stock filename may not be AAPL.csv/TWTR.csv
    if (DEBUG) printf("Loading CSV files...\n");
    store_AAPL_stocks(argv[1]);
    store_TWTR_stocks(argv[2]);
    

    // Step 2: Socket and server setup
    int port = atoi(argv[3]);
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
       
    // Forcefully attaching socket to the port 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(port); 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    if (listen(server_fd, 3) < 0) { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 
    printf("server started\n"); // required print


    // Step 3: Take in commands from client
    char in_msg[MAXCHARS]; char* out_msg;
    char cmd1[MAXCHARS]; char cmd2[MAXCHARS]; char cmd3[MAXCHARS];
    int len;
    int i;
    while (1) {
        // Reset char arrays
        memset(in_msg, 0, MAXCHARS); memset(cmd1, 0, MAXCHARS); memset(cmd2, 0, MAXCHARS); memset(cmd3, 0, MAXCHARS);

        valread = read(new_socket, in_msg, MAXCHARS);

        // Remember that 1st byte is length of string, have to ignore it for cmd1
        char* in_msg_str = in_msg;
        in_msg_str += 1; //skips first char for clean reading

        sscanf(in_msg_str, "%s %s %s", cmd1, cmd2, cmd3);

        /////////////////////

        out_msg = "Unknown"; // default msg

        // Command #1: prices
        if (strcmp(cmd1, "Prices") == 0) 
        { 
            if (strcmp(cmd2, "AAPL") == 0) {
                for (i = 0; i < 504; i++) {
                    if (strcmp(cmd3, AAPL_stocks[i].date) == 0) {
                        char result[50];
                        sprintf(result, "%.2f", AAPL_stocks[i].close);
                        out_msg = result;
                        break;
                    }
                }
            }
            
            
            if (strcmp(cmd2, "TWTR") == 0) {
                for (i = 0; i < 504; i++) {
                    if (strcmp(cmd3, TWTR_stocks[i].date) == 0) {
                        char result[50];
                        sprintf(result, "%.2f", TWTR_stocks[i].close);
                        out_msg = result;
                        break;
                    } 
                }
            }
        }
    

        // Command #2: maxprofit
        else if (strcmp(cmd1, "MaxProfit") == 0) 
        {
            int i, j;
            float profit = 0;
            float stock1, stock2;

            if (strcmp(cmd2, "AAPL") == 0) 
            {    
                for (i = 0; i < 504; i++)
                {
                    stock1 = AAPL_stocks[i].close;
                    for (j = i+1; j < 504; j++) 
                    {
                        stock2 = AAPL_stocks[j].close;
                        if (stock2 <= stock1) continue;
                        if (stock2 - stock1 > profit) profit = stock2 - stock1;
                    }
                }

                char result[50];
                sprintf(result, "Maximum Profit for AAPL: %.2f", profit);
                out_msg = result;
            }

            if (strcmp(cmd2, "TWTR") == 0)
            {
                for (i = 0; i < 504; i++)
                {
                    stock1 = TWTR_stocks[i].close;
                    for (j = i+1; j < 504; j++) 
                    {
                        stock2 = TWTR_stocks[j].close;
                        if (stock2 <= stock1) continue;
                        if (stock2 - stock1 > profit) profit = stock2 - stock1;
                    }
                }

                char result[50];
                sprintf(result, "Maximum Profit for TWTR: %.2f", profit);
                out_msg = result;
            }
        }

        // Command #3: quit
        else if (strcmp(cmd1, "quit") == 0) {
            if (DEBUG) printf("QUIT message received, server quitting...\n");
            break;
        }

        if (strcmp(out_msg, "Unknown") != 0) printf("%s %s %s\n", cmd1, cmd2, cmd3); // required print

        send(new_socket, out_msg, strlen(out_msg), 0); 
        if (DEBUG) printf("%s\n\n", out_msg);
    } // end of while
    return 0; 
} // end of main