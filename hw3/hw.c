// Aljon Viray (86285526) and Daniel Mishkanian (44251598)
// HW 3 for ICS 53 - Harris, Winter 2021

/*
- How to compile and run (do this in openlab in the ~/ics_53/hw3 folder)
clear && gcc hw.c -o hw3 && ./hw3
clear && gcc hw.c -o hw3 && valgrind ./hw3

- How to use line coverage tool (do this in openlab in the ~/ics_53/hw3 folder)
zip hw.zip hw.c test1.run test2.run test3.run test4.run finaltest1.run finaltest2.run
python3 autocov.py > debug_all_output.txt
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MAXLINE 80
#define MAXTIME 255

char input[MAXLINE];
char *argv[3];
char command1[MAXLINE]; char command2[MAXLINE]; char command3[MAXLINE];
int time = 0; int i = 0; int j = 0; int counter = 0;
int DEBUG = 1;

typedef struct PageTableEntry {
    int valid;
    int dirty;
    int page;
} PageTableEntry;
typedef struct MainMemoryPage {
    int addresses[8];
    int is_cached;
    int virt_range[2];
    int time_created;   // used in FIFO
    int last_used;      // used in LRU
} MainMemoryPage;
typedef struct DiskPage {
    int addresses[8];
} DiskPage;

struct PageTableEntry page_table[8] =   {   
                                            {0,0,0},
                                            {0,0,1},
                                            {0,0,2},
                                            {0,0,3},
                                            {0,0,4},
                                            {0,0,5},
                                            {0,0,6},
                                            {0,0,7}  
                                        };
struct MainMemoryPage main_memory[4] =    {   
                                    {{-1, -1, -1, -1, -1, -1, -1, -1}, 0, {-1,-1}, -1, -1},
                                    {{-1, -1, -1, -1, -1, -1, -1, -1}, 0, {-1,-1}, -1, -1},
                                    {{-1, -1, -1, -1, -1, -1, -1, -1}, 0, {-1,-1}, -1, -1},
                                    {{-1, -1, -1, -1, -1, -1, -1, -1}, 0, {-1,-1}, -1, -1}   
                                };
struct DiskPage disk[8] =   {
                            {-1, -1, -1, -1, -1, -1, -1, -1},
                            {-1, -1, -1, -1, -1, -1, -1, -1},
                            {-1, -1, -1, -1, -1, -1, -1, -1},
                            {-1, -1, -1, -1, -1, -1, -1, -1},
                            {-1, -1, -1, -1, -1, -1, -1, -1},
                            {-1, -1, -1, -1, -1, -1, -1, -1},
                            {-1, -1, -1, -1, -1, -1, -1, -1},
                            {-1, -1, -1, -1, -1, -1, -1, -1}
                            };


/////////////////////////////////////////////////////////////////////

// Go thru MM and find the MM-Page with lowest "time_created" value
int FIFO() 
{
    if (DEBUG) printf("Using FIFO to evict an MM-Page...\n");
    int lowest = __INT_MAX__;
    int mm_page_evict = -1;
    for (i = 0; i < 4; i++) {
        if (main_memory[i].time_created < lowest) {
            mm_page_evict = i;
            lowest = main_memory[i].time_created;
        }
    }
    return mm_page_evict;
}

// Go thru MM and find the MM-Page with lowest "last_used" value
int LRU() 
{
    if (DEBUG) printf("Using LRU to evict an MM-Page...\n");
    int lowest = __INT_MAX__;
    int mm_page_evict = -1;
    for (i = 0; i < 4; i++) {
        if (main_memory[i].last_used < lowest) {
            mm_page_evict = i;
            lowest = main_memory[i].last_used;
        }
    }
    return mm_page_evict;
}

void evict_MM_page (int mm_page_evict)
{
    // Find associated PT-Page for this MM-Page
    int pt_page = -1;
    for (i = 0; i < 8; i++) {
        if (page_table[i].valid == 1 && page_table[i].page == mm_page_evict) {
            pt_page = i;
            if (DEBUG) printf("PT-Page #%d currently maps to MM-Page #%d\n", pt_page, mm_page_evict);
            break;
        }
    }

    // Find D-Page to copy MM-Page into
    int disk_page = findDiskPage(main_memory[mm_page_evict].virt_range[0]);
    if (DEBUG) printf("Copying MM-Page #%d to D-Page #%d...\n", mm_page_evict, disk_page);
    for (i = 0; i < 8; i++) {
        disk[disk_page].addresses[i] = main_memory[mm_page_evict].addresses[i];
    }

    // Reset MM-Page
    if (DEBUG) printf("Evicting MM-Page #%d...\n", mm_page_evict);
    for (i = 0; i < 8; i++) {
        main_memory[mm_page_evict].addresses[i] = -1;
    }
    main_memory[mm_page_evict].is_cached = 0;
    main_memory[mm_page_evict].virt_range[0] = -1;
    main_memory[mm_page_evict].virt_range[1] = -1;
    main_memory[mm_page_evict].time_created = -1;
    main_memory[mm_page_evict].last_used = -1;

    // Reset PT-Page
    page_table[pt_page].valid = 0;
    page_table[pt_page].dirty = 0;
    page_table[pt_page].page = disk_page; // Page is now disk's page
    if (DEBUG) printf("MM-Page #%d evicted.\n", mm_page_evict);
}

void cache_MM_page (int virt_addr, int mm_page, int page_table_index)
{
    if (DEBUG) printf("MM-Page #%d available, caching...\n", mm_page);
    page_table[page_table_index].valid = 1; // in main memory = 1, in disk = 0
    page_table[page_table_index].page = mm_page;

    // Store extra info in Main Memory
    main_memory[mm_page].is_cached = 1;
    int low_range = find_MM_range(virt_addr); 
    int high_range = low_range+7;
    main_memory[mm_page].virt_range[0] = low_range; 
    main_memory[mm_page].virt_range[1] = high_range;
    main_memory[mm_page].time_created = time;
}

// Print FULL current state of main memory in 4 by 8 matrix
void printMainMemory()
{
    for (i = 0; i < 4; i++) {
        MainMemoryPage mm_page = main_memory[i];
        for (j = 0; j < 8; j++) {
            printf("%d, ", mm_page.addresses[j]);
        }
        printf("| is_cached = %d\t", mm_page.is_cached);
        printf("| virt_range = %d to %d\t", mm_page.virt_range[0], mm_page.virt_range[1]);
        printf("| time_created = %d\t", mm_page.time_created);
        printf("| last_used = %d\n", mm_page.last_used);
    }
}

// Check which range of virt_addresses an MM_Page should hold, used when caching an new MM-Page
int find_MM_range (int virt_addr) 
{
    int temp = -1;
    if (virt_addr >= 0 && virt_addr <= 7)   temp = 0;
    if (virt_addr >= 8 && virt_addr <= 15)  temp = 8;
    if (virt_addr >= 16 && virt_addr <= 23) temp = 16;
    if (virt_addr >= 24 && virt_addr <= 31) temp = 24;
    if (virt_addr >= 32 && virt_addr <= 39) temp = 32;
    if (virt_addr >= 40 && virt_addr <= 47) temp = 40;
    if (virt_addr >= 48 && virt_addr <= 55) temp = 48;
    if (virt_addr >= 56 && virt_addr <= 63) temp = 56;;
    return temp;
}

// Check which array in Disk to copy into from MM, used when caching an new MM-Page
int findDiskPage (int low_range) 
{
    int temp = -1;
    if (low_range == 0)  temp = 0;
    if (low_range == 8)  temp = 1;
    if (low_range == 16) temp = 2;
    if (low_range == 24) temp = 3;
    if (low_range == 32) temp = 4;
    if (low_range == 40) temp = 5;
    if (low_range == 48) temp = 6;
    if (low_range == 56) temp = 7;
    return temp;
}


/////////////////////////////////////////////////////////////////////


int main (int argc, char *argv[]) 
{
    while (time <= MAXTIME) 
    {
        // Reset variables, so prompt with ignore blank inputs
        strcpy(command1, ""); strcpy(command2, ""); strcpy(command3, "");

        // Take in input, Tokenize/Parse it
        if (DEBUG) printf("Time = %d\n", time);
        printf("> ");
        fgets(input, MAXLINE, stdin);
        sscanf(input, "%s %s %s", command1, command2, command3);

        // #1 - Read
        if (strcmp(command1, "read") == 0) {
            // Find Page Table index, using first three bits of virt_addr with binary mask
            int virt_addr = atoi(command2);
            int number = atoi(command3);
            int page_table_index = (virt_addr & 0b111000) / 8;
            if (DEBUG) printf("Checking Page Table #%d...\n", page_table_index);

            // Check where the virt_addr belongs in Main Memory (MM) by looking through Page Table (PT)
            int mm_page = page_table[page_table_index].page;
            if (page_table[page_table_index].valid == 1 && mm_page >= 0 && mm_page <= 3) {
                if (DEBUG) printf("Page #%d is already in Main Memory!\n", page_table_index); // Skip to end where we WRITE
            }

            else {
                printf("A Page Fault Has Occured\n");
                // Check if we should cache a new MM-Page or use FIFO/LRU
                int empty_mm_page = -1;
                for (i = 0; i < 4; i++) {
                    if (main_memory[i].is_cached == 0) { // its not cached yet, use this one!
                        empty_mm_page = i;
                        break;
                    } 
                }

                // Cache this MM-Page by storing its info in PT
                if (empty_mm_page > -1) {
                    cache_MM_page(virt_addr, empty_mm_page, page_table_index);
                    mm_page = empty_mm_page;
                }

                // Otherwise, if all of MM-Pages are already cached, use FIFO/LRU to make room in MM
                else {
                    if (DEBUG) printf("Can't cache, all MM-Pages are full!\n");
                    int mm_page_evict = -1;

                    // Default to FIFO, or use FIFO if given
                    if (argc < 2 || strcmp(argv[1], "FIFO") == 0) {
                        mm_page_evict = FIFO();
                    }
                    // Use LRU if given
                    else if (strcmp(argv[1], "LRU") == 0) {
                        mm_page_evict = LRU();
                    }

                    evict_MM_page(mm_page_evict);
                    cache_MM_page(virt_addr, mm_page_evict, page_table_index);
                    mm_page = mm_page_evict;
                }
            }

            // After PAGE HIT OR PAGE FAULT, Use last 3 bits of command2 binary to READ FROM index in MM-page
            if (DEBUG) printf("Main Memory Index = %d\n", (virt_addr & 0b000111));
            printf("%d\n", main_memory[mm_page].addresses[(virt_addr & 0b000111)]);
            main_memory[mm_page].last_used = time; // For both reading and writing

            if (DEBUG) printMainMemory();
        }

        // #2 - Write
        else if (strcmp(command1, "write") == 0) {
            // Find Page Table index, using first three bits of virt_addr with binary mask
            int virt_addr = atoi(command2);
            int number = atoi(command3);
            int page_table_index = (virt_addr & 0b111000) / 8;
            if (DEBUG) printf("Checking Page Table #%d...\n", page_table_index);

            // Check where the virt_addr belongs in Main Memory (MM) by looking through Page Table (PT)
            int mm_page = page_table[page_table_index].page;
            if (page_table[page_table_index].valid == 1 && mm_page >= 0 && mm_page <= 3) {
                if (DEBUG) printf("Page #%d is already in Main Memory!\n", page_table_index); // Skip to end where we WRITE
            }

            else {
                printf("A Page Fault Has Occured\n");
                // Check if we should cache a new MM-Page or use FIFO/LRU
                int empty_mm_page = -1;
                for (i = 0; i < 4; i++) {
                    if (main_memory[i].is_cached == 0) { // its not cached yet, use this one!
                        empty_mm_page = i;
                        break;
                    } 
                }

                // Cache this MM-Page by storing its info in PT
                if (empty_mm_page > -1) {
                    cache_MM_page(virt_addr, empty_mm_page, page_table_index);
                    mm_page = empty_mm_page;
                }

                // Otherwise, if all of MM-Pages are already cached, use FIFO/LRU to make room in MM
                else {
                    if (DEBUG) printf("Can't cache, all MM-Pages are full!\n");
                    int mm_page_evict = -1;

                    // Default to FIFO, or use FIFO if given
                    if (argc < 2 || strcmp(argv[1], "FIFO") == 0) {
                        mm_page_evict = FIFO();
                    }
                    // Use LRU if given
                    else if (strcmp(argv[1], "LRU") == 0) {
                        mm_page_evict = LRU();
                    }
                    
                    evict_MM_page(mm_page_evict);
                    cache_MM_page(virt_addr, mm_page_evict, page_table_index);
                    mm_page = mm_page_evict;
                }
            }

            // After PAGE HIT OR PAGE FAULT, Use last 3 bits of command2 binary to WRITE TO index in MM-page
            // Note: It is OK to overwrite an existing integer if this is an already-cached MM-Page
            if (DEBUG) printf("Main Memory Index = %d\n", (virt_addr & 0b000111));
            main_memory[mm_page].addresses[(virt_addr & 0b000111)] = number;
            main_memory[mm_page].last_used = time; // For both reading and writing
            page_table[page_table_index].dirty = 1; // Only for writing

            if (DEBUG) printMainMemory();
        }

        // #3 - Show Main Memory
        else if (strcmp(command1, "showmain") == 0) {
            int mm_page = atoi(command2);
            for (i = 0; i < 8; i++) {
                printf("%d:%d\n", (8*mm_page+i), main_memory[mm_page].addresses[i]);
            }
        }

        // #4 - Show Disk
        else if (strcmp(command1, "showdisk") == 0) {
            int dp = atoi(command2);
            for (i = 0; i < 8; i++) {
                printf("%d:%d\n", (8*dp+i), disk[dp].addresses[i]);
            }
  
            // [DEBUG] Print all of disk
            if (DEBUG) {
                for (i = 0; i < 8; i++) {
                    for (j = 0; j < 8; j++) {
                            printf("%d\t", disk[i].addresses[j]);
                    }
                    printf("\n");
                }
            }
        }

        // #5 - Show Page Table
        else if (strcmp(command1, "showptable") == 0) {
            for (i = 0; i < 8; i++) {
                printf("%d:%d:%d:%d\n", i, page_table[i].valid, page_table[i].dirty, page_table[i].page);
            }
        }

        // #6 - Quit
        else if (strcmp(command1, "quit") == 0) {
            break;
        }

        if (DEBUG) printf("\n");
        time++;
    } // end of while loop
    return 0;
} // end of main()