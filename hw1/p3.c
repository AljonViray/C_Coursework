#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Event {
    char* name;
    int start;
    int end;
} Event;

int main () 
{
    char str[80], command[80], name[80];
    int i=0, start=0, end=0, numEvents=0, curTime=0, numPrinted=0, low=0, high=0;
    struct Event *calendar = malloc(24 * sizeof(Event));

    while (1) {
        printf("$ ");
        fgets(str, sizeof(str), stdin);
        // Tokenize string
        sscanf(str, "%s %s %d %d", command, name, &start, &end);  

        // Quit
        if (strcmp(command, "quit") == 0) {
            for (i = 0; i < numEvents; i++) {
                free(calendar[i].name);
            }
            free(calendar);
            break;
        }

        // Add event
        else if (strcmp(command, "add") == 0) {
            // Check if we are allowed to add this event:
            if ((start == 0 && end == 0) || start < 0 || end > 23) {
                continue;
            }
            bool badInput = false;
            for (i=0; i < numEvents; i++) {
                low = calendar[i].start;
                high = calendar[i].end;
                // Good cases:
                    // Start & End are both before existing event(s), Start & End are both after existing event(s),
                    // Start < Low/End == High, Start == Low/End > High
                if ((start < low && end <= low) || (start >= high && end > high)) {
                    // passed the test, just keep looping
                }
                else {
                    printf("Event overlap error\n");
                    badInput = true;
                    break;
                }
            }
            if (badInput == true) {
                badInput = false;
                continue;
            }
            // Add event to calendar if conditions above passed
            struct Event *e = malloc(sizeof(Event));
            e->name = malloc(strlen(name) + 1);
            strcpy(e->name, name);
            e->start = start;
            e->end = end;
            calendar[numEvents] = *e;
            numEvents++;
            free(e);
        }

        // Delete event
        else if (strcmp(command, "delete") == 0) {
            if (numEvents > 0)
            {
                int indexToDelete = 0;
                // Find the index of the Event we want to delete
                for (i = 0; i < numEvents; i++) {
                    if (strcmp(calendar[i].name, name) == 0) {
                        indexToDelete = i;
                        // free() the string from indexToDelete in the array
                        free(calendar[indexToDelete].name);
                        // Overwrite the Event to "delete", move all Events after this one down the array
                        for (i = indexToDelete; i < numEvents-1; i++) {
                            calendar[i] = calendar[i+1];
                        }
                        numEvents--;
                        break;
                    }
                }
            }
        }

        else if (strcmp(command, "printcalendar") == 0) {
            // Print in sorted order by start_time. Just print all 0's, 1's, 2's, etc in order.
            i = 0, numPrinted = 0, curTime = 0;
            while (numPrinted != numEvents && curTime < 24) {
                // Print if curTime matches start_time
                if (calendar[i].start == curTime) {
                    printf("%s %d %d\n", calendar[i].name, calendar[i].start, calendar[i].end);
                    numPrinted++;
                    i++;
                }
                // Keep looping around the calendar list
                else if (calendar[i].start != curTime) {
                    i++;
                }
                // Reset loop to look for more items to print, from 0 to 23
                if (i == numEvents && numPrinted != numEvents) {
                    curTime++;
                    i = 0;
                }
            }
        }

        else {
            printf("Invalid command, please try again.\n");
        }
    }

    return 0;
}