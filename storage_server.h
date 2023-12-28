#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

#define NM_IP "127.0.0.1" // Replace with the IP of the Naming Server
#define NM_PORT 8888      // Naming Server's dedicated port
#define MAX_PATHS 100
#define MAX_CLIENTS 20
#define CHUNK_SIZE 100
#define MAX_SUM_OF_ALL_PATHS 4096
#define MAX_REGISTER_SIZE 8192

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

struct file_access
{
    char filepath[4096];
    sem_t readlock;
    sem_t writelock;
    int readercount;
};

char *read_file(char *path);
int write_to_file(char *path, char *contents);
struct size_per *get_file_size_per(char *path);
int createEmptyFile(char *filename);
int createEmptyDirectory(char *filename);
int deleteFile(char *filename);
int deleteDirectory(char *dirname);
void HandleErrors(int errno);