#include "defs.h"
#include <stdio.h>

void HandleErrors(int errno)
{
    if(errno == 1)
    {
        printf(RED "Permission Denied" WHITE "\n");
    }
    else if(errno == 2)
    {
        printf(RED "Not Found" WHITE "\n");
    }
    else if(errno == 3)
    {
        printf(RED "Failed to perform operation" WHITE "\n");
    }
    else if(errno == 4)
    {
        printf(RED "Syntax Error" WHITE "\n");
    }
    else if(errno == 5)
    {
        printf(RED "Network Error" WHITE "\n");
    }
    else if(errno == 6)
    {
        printf(RED "Timeout" WHITE "\n");
    }
    else if(errno == 7)
    {
        printf(RED "Communication Error" WHITE "\n");
    }
}