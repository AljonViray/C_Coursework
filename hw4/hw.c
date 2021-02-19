// Aljon Viray (86285526) and Daniel Mishkanian (44251598)
// HW 4 for ICS 53 - Harris, Winter 2021

/*
- How to compile and run (do this in openlab in the ~/ics_53/hw3 folder)
clear && gcc hw.c -o hw4 && ./hw4
clear && gcc hw.c -o hw4 && valgrind ./hw4

- How to use line coverage tool (do this in openlab in the ~/ics_53/hw3 folder)
zip hw.zip hw.c test1.run test2.run test3.run test4.run
python3 autocov.py > debug.txt
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MAXLINE 80

char input[MAXLINE];
char *argv[3];
char command1[MAXLINE]; char command2[MAXLINE]; char command3[MAXLINE];
unsigned char heap[127] = {}; // We don't keep track which slots are header/footer/payload in code. Keep track in your head.
int i = 0;
int DEBUG = 0;


////////////////////////////////////////////////////////////////////////


// [DEBUG ONLY] Returns a string of chars representing binary of the unsigned char input 'x'
// const char* byte_to_binary (unsigned char c)
// {
//     static char binary[9];
//     binary[0] = '\0';
//     int i;
//     for (i = 7; i >= 0; --i) {
//         strcat(binary, (c & (1 << i)) ? "1" : "0"); // Decides to concatonate 0 or 1 to binary string
//     }
//     return binary;
// }

// Combines two or more free blocks together after a free() command happens
void coalesce_free_blocks (int header_index, int footer_index)
{
    // Cases to DO NOT use coalesce_free_blocks()
    if (header_index == 0 && footer_index == 126) return; // If we freed the entire heap as a single block
    if (header_index == 0 && (heap[footer_index+1] & 0b00000001) == 1) return; // If we freed a far LEFT block with an allocated neighbor block
    if (footer_index == 126 && (heap[header_index-1] & 0b00000001) == 1) return; // If we freed a far RIGHT block with an allocated neighbor block
    if ((heap[footer_index+1] & 0b00000001) == 1 && (heap[header_index-1] & 0b00000001) == 1) return; // If we freed a middle block with 2 allocated neighbor blocks

    // Cases we DO use coalesce_free_blocks()
    // We are in the [LEFT] block. We want to combine with the Right block.
    if ((header_index == 0 || (heap[header_index-1] & 0b00000001) == 1) && (heap[footer_index+1] & 0b00000001) == 0 )
    {
        if (DEBUG) printf("[LEFT] free block combine with Right free block\n");
        unsigned char right_header_value = heap[footer_index+1] >> 1;

        // Change the left-most header to the new value ([LEFT] block size + right block size)
        heap[header_index] = ( (heap[header_index] >> 1) + right_header_value ) << 1;
        
        // Copy new block size value to the right-most footer
        heap[footer_index + right_header_value] = heap[header_index]; 

        // Delete the old [LEFT] block's footer and old Right block's header, they are in the middle of the new block
        heap[footer_index] = 0b00000000;
        heap[footer_index+1] = 0b00000000;
    }

    //We are in the [RIGHT] block. Want to combine with the left block.
    else if ( (footer_index == 126 || (heap[footer_index+1] & 0b00000001) == 1 ) && ( heap[header_index-1] & 0b00000001 ) == 0 )
    {
        if (DEBUG) printf("[RIGHT] free block combine with Left free block\n");
        unsigned char left_footer = heap[header_index-1] >> 1;

        // Change the left-most header to the new value ([RIGHT] block size + Left block size)
        heap[header_index - left_footer] = ( left_footer + (heap[header_index] >> 1) ) << 1;
        
        // Copy new block size value to the right-most footer
        heap[footer_index] = heap[header_index - left_footer];

        // Delete the old [RIGHT] block's header and the old left block's, they are in the middle of the new block
        heap[header_index] = 0b00000000;
        heap[header_index-1] = 0b00000000;
    }

    //We are in between 2 free blocks. Combine these 3.
    else if (( heap[header_index-1] & 0b00000001 ) == 0 && ( heap[footer_index+1] & 0b00000001 ) == 0) 
    {
        if (DEBUG) printf("[MIDDLE] free block combine with Left and Right free blocks\n");
        unsigned char middle_header = heap[header_index] >> 1;
        unsigned char right_header_value = heap[footer_index+1] >> 1;
        unsigned char left_footer = heap[header_index-1] >> 1;

        // Change the left-most header to the new value (Left block size + [MIDDLE] block size + Right block size)
        heap[header_index - left_footer] = (left_footer + middle_header + right_header_value) << 1;

        // Copy new block size value to the right-most footer
        heap[footer_index + right_header_value] = heap[header_index - left_footer];

        // Delete old Left block's footer, old [MIDDLE] block's header & footer, and old Right block's header
        heap[header_index-1] = 0b00000000;
        heap[header_index] = 0b00000000;
        heap[footer_index] = 0b00000000;
        heap[footer_index+1] = 0b00000000;
    }

} 

////////////////////////////////////////////////////////////////////////



int main() 
{
    // The header and footer Slot this block as "127 bytes that are free"
    // Note: integer '127' = binary '0b1111111[0/1]'
    // Initialize the heap by making header and footer around it, heap starts as free block
    heap[0] = (127 << 1);   // header, w/ binary mask for allocation bit (last bit = 0)
    heap[126] = (127 << 1); // footer, w/ binary mask for allocation bit (last bit = 0)

    // Print binary of the header/footer we just stored
    //if (DEBUG) printf("heap[0] Header initialized as %s\n", byte_to_binary(heap[0]));
    //if (DEBUG) printf("heap[126] Footer initialized as %s\n", byte_to_binary(heap[126]));


    while(1) 
    {
        // Reset variables, so prompt with ignore blank inputs
        strcpy(command1, ""); strcpy(command2, ""); strcpy(command3, "");

        // Take in input, Tokenize/Parse it
        printf(">");
        fgets(input, MAXLINE, stdin);
        sscanf(input, "%s %s %s", command1, command2, command3);

        ////////////////////////////////////////

        // #1 - malloc <int size>
        if (strcmp(command1, "malloc") == 0) {
            int size_needed = atoi(command2) + 2; // 2 extra slots for the header and footer
            unsigned char free_header;
            int free_header_old_index;
            int free_footer_i;

            // Step 1: Check if it is a free block that is big enough for this malloc()
            for (i = 0; i < 127; i++) {
                // Check if "is_allocated" bit is 0 (free block header), Check if real value of the header is enough space by right-shifting the bits
                if (DEBUG) printf("Checking if heap[%d] is a big enough free block header...\n", i);
                if ( (heap[i] & 0b00000001) == 0 && (heap[i] >> 1) >= size_needed ) { 
                    if (DEBUG) printf("heap[%d] is a valid header!\n", i);
                    free_header = heap[i];
                    free_header_old_index = i;
                    i = (free_header >> 1) - 1 + i; // Get the first 7 bits to read address value, "teleport" to footer of this block
                    free_footer_i = i;
                    
                    if (DEBUG) printf("Checking associated footer at heap[%d]...\n", i);
                    unsigned char free_footer = heap[i];

                    // if (free_header == free_footer) { 
                    //     if (DEBUG) printf("Footer is valid, moving on to allocation...\n");
                    //     break;
                    // }
                    // else {
                    //     if (DEBUG) printf("Footer is invalid, finding next free block...\n");
                    // }
                }
            }

            // Step 2: Move the free block's header to accomodate for the malloc() block
            if (DEBUG) printf("Moving free block header to new address to make room for allocated block...\n");
            int free_header_new_i = free_header_old_index + size_needed;
            heap[free_header_new_i] = ((free_header >> 1) - size_needed) << 1;
            heap[free_footer_i] = ((free_header >> 1) - size_needed) << 1;
            //if (DEBUG) printf("New free block header at heap[%d] = %s\n", free_header_new_i, byte_to_binary(heap[free_header_new_i]));
            //if (DEBUG) printf("New free block footer at heap[%d] = %s\n", free_footer_i, byte_to_binary(heap[free_footer_i]));

            // Step 3: Assign new header and footer to correct values (the new block size)
            if (DEBUG) printf("Creating allocated block...\n");
            heap[free_header_old_index] = (size_needed << 1) + 1;   // header, w/ binary mask for allocation bit (last bit = 1)
            heap[free_header_old_index + size_needed - 1] = (size_needed << 1) + 1;  // footer, w/ binary mask for allocation bit (last bit = 1)
            
            // Step 4: Print the first address of the newly created block (the index which we can free() at)
            printf("%d\n", free_header_old_index+1);

            // Print binary of the header/footer we just stored
            //if (DEBUG) printf("heap[%d] Header set to %s\n", free_header_old_index, byte_to_binary(heap[free_header_old_index]));
            //if (DEBUG) printf("heap[%d] Footer set to %s\n", size_needed, byte_to_binary(heap[size_needed]));
        }


        // #2 - free <int size>
        else if (strcmp(command1, "free") == 0) {
            int index = atoi(command2);
            unsigned char header_value;
            int header_index;
            int footer_index;

            //check to see if block (from header) is free or not
            if ( (heap[index-1] & 0b00000001) != 0 ) { 
                header_value = (heap[index-1] >> 1);
                header_index = index-1; // Header should be to the left of given index
                footer_index = header_index + header_value - 1;
                
                // Start freeing (turning into 0's) addresses from header to footer
                for (i = header_index+1; i < footer_index; i++) {
                    heap[i] = 0b00000000;
                }
                // Mark header and footer themselves as "free"
                heap[header_index] -= 1;
                heap[footer_index] -= 1;

                // Try to combine free block we just made with existing free block(s)
                coalesce_free_blocks(header_index, footer_index);

                //if (DEBUG) printf("Changed header to %s\n", byte_to_binary(heap[header_index]));
                //if (DEBUG) printf("Changed footer to %s\n", byte_to_binary(heap[footer_index]));
            }
        }


        // #3 - blocklist
        else if (strcmp(command1, "blocklist") == 0) {
            // Print every block: "<first index of block's payload>, <size of block's payload>, <block status>"
            for (i = 0; i < 127; i++) {
                // Print the info of this block
                char* status;
                if ((heap[i] & 0b00000001) == 0) {
                    status = "free.";
                }
                else if ((heap[i] & 0b00000001) == 1) {
                    status = "allocated.";
                }
                printf("%d, %d, %s\n", i+1, (heap[i] >> 1) - 2, status);

                // Teleport to next block
                i = i + (heap[i] >> 1) - 1;
            }
        }


        // #4 - writemem <int index> <char* str>
        else if (strcmp(command1, "writemem") == 0) {
            int index = atoi(command2);
            int len = strlen( command3 );
            for (i = 0; i < len; i++) {
                heap[index+i] = command3[i];
            }
        }


        // #5 - printmem <int index> <int num_of_chars_to_print>
        else if (strcmp(command1, "printmem") == 0) {
            int index = atoi(command2);
            int num_char = atoi(command3);
            for (i = index; i < index+num_char; i++) {
                printf("%x ", heap[i]);
            }
            printf("\n");
        }


        // #6 - Quit
        else if (strcmp(command1, "quit") == 0) { break; }


        // DEBUG - Print entire heap
        // if (DEBUG) {
        //     int counter = 0;
        //     for (i = 0; i < 127; i++) {
        //         if (counter % 8 == 0) {
        //             printf("\n");
        //             counter = 0;
        //         }
        //         printf("%s\t", byte_to_binary(heap[i]));
        //         counter++;
        //     }
        //     printf("\n");
        // }

    } // end of while
    return 0;
} // end of main