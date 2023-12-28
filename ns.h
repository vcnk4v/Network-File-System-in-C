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

#define READ 20
#define ALPHABET_SIZE 68
#define BUFFER_SIZE 4096
#define CHUNK_SIZE 100
#define CACHE_SIZE 10
#define MAX_SUM_OF_ALL_PATHS 4096
#define MAX_CLIENTS 20
#define MAX_PATHS 100

#define NM_IP "127.0.0.1"

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

struct trie_node
{
    int SS_index;                              // store primary SS and 2 backups
    struct trie_node *children[ALPHABET_SIZE]; // 52 alphabets + 10 numbers + '/' + '.'
};
typedef struct trie_node *Trie;

typedef struct
{
    char filepath[4096];
    SS ss;
    int last_used;
} CacheEntry;

struct port
{
    int port;
    int free;
};

extern SS storage_server;
extern SS storage_server_tail;

SS Make_Node_SS(char *buffer, int server);
int Make_Socket_For_SS(SS storage_server_details);

SS find_ss_by_port(int port_no);
Trie make_node(int SS_index1);
int insert(char *str, int len, int SS_index);
int search_trie(char *str, int len); // returns indices of SS and the two backup SS
SS search_helper_trie(char *path, int operation_flag);
int delete_folder(char *fname);
int delete_file(char *path);
int create_file(int ss_index, char *path);
int create_folder(int ss_index, char *fname);

extern Trie accessible_paths_trie;

char **subpaths_in_trie(int *num_paths, char *parent);
void get_all_paths(Trie root, char *prefix, int *num_paths, char **subpaths);
char get_char_from_index(int index);
SS search_backups(SS unavailable_ss);

void *send_periodic_message_to_active_servers();
void send_message_to_active_servers();
char *Handle_Read_Request(int ss_socket, int storage_server_socket_dest, char *newfile_path);

extern CacheEntry cache[CACHE_SIZE];
void initializeCache();
SS search_helper(char *path, int operation_flag);

void log_client_request(char *client_ip, int client_port, int ss_index, char *request, char *acknowledgement);
char *extract_filename(char *src_dir);
void duplicate_accessible_paths(SS ss);
void Copy_File_redundancy(char *buffer, SS original, SS backup);
void extractSubstring(const char *source, char *destination);
void get_all_path_depth(Trie root, char *prefix, int *num_paths, char **subpaths, int ss_index);
void HandleErrors(int errno);
extern struct port all_ports[20];
void Copy_Folder_redundancy(char *src_dir, SS original, SS backup);