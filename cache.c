#include "ns.h"
#include "time.h"
CacheEntry cache[CACHE_SIZE];
time_t start;

void initializeCache()
{
    time(&start);
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        memset(cache[i].filepath, 0, sizeof(cache[i].filepath));
        cache[i].ss = NULL;
        cache[i].last_used = 0; // Initialize with 0
    }
}

void updateCache(char *filepath, SS ss)
{
    // pthread_mutex_lock(&cache_mutex);
    int index = -1;
    int min_last_used = cache[0].last_used;

    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (strlen(cache[i].filepath) == 0)
        {
            index = i;
            break;
        }
        if (cache[i].last_used <= min_last_used)
        {
            min_last_used = cache[i].last_used;
            index = i;
        }
    }

    if (index >= 0)
    {
        strncpy(cache[index].filepath, filepath, sizeof(cache[index].filepath));
        cache[index].ss = ss;

        cache[index].last_used = (int)(time(NULL) - start); // Update the last used time as an int
    }
    // pthread_mutex_unlock(&cache_mutex);
}

SS search_helper(char *path, int operation_flag)
{
    int length_to_compare = 0;
    if (operation_flag == 0) // delete, read, write etc
    {
        length_to_compare = strlen(path);
    }
    else if (operation_flag == 2) // create folder
    {
        char temp[4096];
        strncpy(temp, path, strlen(path) - 1);
        char *lastSlash = strrchr(temp, '/');
        length_to_compare = lastSlash - temp + 1;
    }
    else if (operation_flag == 1) // create file
    {
        char *lastSlash = strrchr(path, '/');
        length_to_compare = lastSlash - path + 1;
    }

    // pthread_mutex_lock(&cache_mutex);
    int i;
    for (i = 0; i < CACHE_SIZE; i++)
    {
        if (strncmp(cache[i].filepath, path, length_to_compare) == 0)
        {
            cache[i].last_used = (int)time(NULL); // Update the last used time as an int
            printf("Cache hit! In ss: %d\n", cache[i].ss->SS_index);
            return cache[i].ss;
            // pthread_mutex_unlock(&cache_mutex);
        }
    }

    // If the file is not found in the cache, search for it and update the cache
    SS ss_for_file = search_helper_trie(path, operation_flag);

    if (ss_for_file)
    {
        printf("Cache miss! In ss: %d\n", ss_for_file->SS_index);

        updateCache(path, ss_for_file);
        return ss_for_file;
    }
    // Return -1 if the file is not found
    return NULL;
}

// int main_func()
// {

//     char accessiblePaths[10][4096];
//     strcpy(accessiblePaths[0], "dir");
//     strcpy(accessiblePaths[1], "dira");
//     strcpy(accessiblePaths[2], "file1");

//     storage_server = (SS)malloc(sizeof(struct SS_Info));
//     storage_server->SS_index = 0;
//     storage_server->next = NULL;
//     storage_server_tail = storage_server;
//     char buffer[4096];
//     sprintf(buffer, "REGISTER\n%s\n%d\n%d\n%s %s %s", "1271", 1, 2, accessiblePaths[0], accessiblePaths[1], accessiblePaths[2]);

//     storage_server_tail->next = Make_Node(buffer);
//     storage_server_tail = storage_server_tail->next;
//     storage_server_tail->socket = -1;

//     initializeCache();

//     char penis[100];
//     while (1)
//     {
//         scanf("%s", penis);
//         searchCache(penis, 0);
//     }
// }