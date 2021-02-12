#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int main()
{
    int i = 0;
    char str[80], str2[80];
    fgets(str, sizeof(str), stdin);

    // Copy string to other variable for printing later
    strcpy(str2, str);

    // Tokenize and print tokens
    char *token = strtok(str, " \t\n");
    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, " \t\n");
    }
    
    printf("%s", str2);
}