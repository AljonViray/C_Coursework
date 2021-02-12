/*
How to compile and run (do this in openlab in the ~/ics_53/hw2 folder)
clear && gcc counter.c -o counter && ./counter
clear && gcc counter.c -o counter && valgrind ./counter
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For use with hw2.c
// Turns input string argv[1] into number, adds 1 to it, then prints it.
int main (int argc, char *argv[]) 
{
    unsigned int i = 0;
    while(1)
    {
        printf("Counter: %d\n", i);
        i++;
        sleep(2);
    }
    return 0;
}
