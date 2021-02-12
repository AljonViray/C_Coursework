#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

int main()
{
    int i, j, randNum = 0;
    int array[11] = {0};

    // Record 100 random numbers from 0 to 12
    for (i = 0; i < 100; i++) {
        randNum = rand() % (12-2 + 1) + 2;
        array[randNum-2]++;
    }

    for (i = 2; i < 13; i++) {
        printf("%d: %d \t", i, array[i-2]);
        for (j = 0; j < array[i-2]; j++) {
            putchar('*');
        }
        putchar('\n');
    }
}