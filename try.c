#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <strings.h>
#define ALPHABET_SIZE 68
typedef struct SS_Info *SS;
char *create_folders_inner_red(char *buffer, char *dir_made_till_now)
{
    // dirc3/dird3/file2   dira3/dirc3/dird3/
    char copy[4096];
    strcpy(copy, buffer);

    char *made = (char *)malloc(sizeof(char) * 1024);
    strcpy(made, "");

    char *made_dup = (char *)malloc(sizeof(char) * 1024);
    strcpy(made_dup, "");

    char *tok = strtok(copy, "/");

    while (tok != NULL)
    {
        // printf("%s\n", tok);
        char *found = strstr(dir_made_till_now, made_dup);
        if (found == NULL)
        {
            printf("%s\n", made);
            return made;
        }
        else // made = dirc3/dird3/
        {
            strcat(made_dup, "/");
            strcpy(made, made_dup);
            tok = strtok(NULL, "/");
        }
    }
    return made;
}

// int main()
// {
//     while (1)
//     {
//         char str[256];
//         char str2[256];
//         scanf("%s", str);
//         scanf("%s", str2);
//         char *made = create_folders_inner_red(str, str2);
//         printf("%s\n", made);
//     }

//     return 0;
// }