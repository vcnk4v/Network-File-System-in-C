#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define BUFFER_SIZE 10

typedef struct SS_Info *SS;

struct SS_Info
{
    int socket;
    int SS_index;
    SS backups[2];
    char IP_addr[32];
    int SS_Port;
    int NM_Port;
    int is_active;
    SS next;
};

struct size_per
{
    off_t file_size;
    mode_t file_permissions;
};
int Make_Socket_For_SS(SS storage_server_details);
void HandleErrors(int errno);