/*
How to compile and run (do this in openlab in the ~/ics_53/hw2 folder)
clear && gcc add.c -o add && ./add 1
clear && gcc add.c -o add && valgrind ./add 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For use with hw2.c
// Turns input string argv[1] into number, adds 2 to it, then prints it.
int main (int argc, char *argv[]) 
{
    int n = atoi(argv[1]);
    printf("%d \n", n+2);
    return 0;
}