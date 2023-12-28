#include "ns.h"
#include "defs.h"

#define NM_IP "127.0.0.1"
#define CLIENT_PORT 8080
#define SERVER_PORT 8888
#define MAX_FILES 10

struct port all_ports[20];

SS storage_server;
Trie accessible_paths_trie;
SS storage_server_tail;

struct command_ss
{
    SS ss;
    char command[4096];
    int cl;
};

pthread_t all_client_threads[MAX_CLIENTS];
int client_index = 0;

void initialise_all_ports_array()
{
    int port1 = 12345;
    for (int i = 0; i < 20; i++)
    {
        all_ports[i].port = port1 + i;
        all_ports[i].free = 1;
    }
}

int return_free_port()
{
    for (int i = 0; i < 20; i++)
    {
        if (all_ports[i].free == 1)
        {
            return i;
        }
    }
    return -1;
}

void handle_server_requests(int server)
{
    char buffer[4096];
    bzero(buffer, sizeof(buffer));
    if (recv(server, buffer, sizeof(buffer), 0) < 0)
    {
        perror("Error in recieving request from storage server");
        exit(EXIT_FAILURE);
    }

    printf("%s..\n", buffer);

    if (strncmp(buffer, "REGISTER", 8) == 0)
    {
        char message[128];
        bzero(message, sizeof(message));
        printf("Registering!\n");
        storage_server_tail->next = Make_Node_SS(buffer, server);
        if (!(storage_server_tail->next))
        {
            strcpy(message, "exists");
            if (send(server, message, sizeof(message), 0) < 0)
            {
                perror("Error in confirming registration");
            }
            printf("Already exists\n");
            return;
        }
        strcpy(message, "registered");
        // if (send(server, message, sizeof(message), 0) < 0)
        // {
        //     perror("Error in confirming registration");
        // }
        storage_server_tail = storage_server_tail->next;

        char **all_ss_paths = (char **)malloc(sizeof(char) * MAX_PATHS);

        for (int i = 0; i < MAX_PATHS; i++)
        {
            all_ss_paths[i] = (char *)malloc(sizeof(char) * 2048);
            bzero(all_ss_paths[i], sizeof(all_ss_paths[i]));
        }

        int n;

        get_all_path_depth(accessible_paths_trie, "", &n, all_ss_paths, storage_server_tail->SS_index);

        char all_paths[MAX_SUM_OF_ALL_PATHS] = "";
        // bzero(all_paths,sizeof(all_paths));

        // printf("%d\n",n);

        for (int i = 0; i < n; i++)
        {
            strcat(all_paths, all_ss_paths[i]);
            strcat(all_paths, "\n");
        }

        send(server, all_paths, sizeof(all_paths), 0);

        if (storage_server_tail->SS_index == 3)
        {
            // storage_server->next->backups[0] = storage_server->next->next;
            // storage_server->next->backups[1] = storage_server_tail;

            // storage_server->next->next->backups[0] = storage_server->next;
            // storage_server->next->next->backups[1] = storage_server_tail;

            storage_server_tail->backups[0] = storage_server->next;
            storage_server_tail->backups[1] = storage_server->next->next;

            SS new_ss = storage_server_tail;
            duplicate_accessible_paths(new_ss);
            // new_ss = storage_server->next->next;
            // duplicate_accessible_paths(new_ss);
            // new_ss = storage_server->next;
            // duplicate_accessible_paths(new_ss);
        }
        else if (storage_server_tail->SS_index > 3)
        {
            SS new_ss = storage_server_tail;
            duplicate_accessible_paths(new_ss);
        }
    }
}

void *handle_storage_servers()
{
    int storage_server_socket, storage_server_connection;
    struct sockaddr_in storage_server_addr, storage_diff_addr;
    socklen_t storage_diff_addr_len;

    storage_server = (SS)malloc(sizeof(struct SS_Info));
    storage_server->SS_index = 0;
    storage_server->next = NULL;
    storage_server_tail = storage_server;

    accessible_paths_trie = make_node(-1); // trie

    FILE *log_file = fopen("bookkeeping_log.txt", "w");
    fclose(log_file);
    initializeCache();
    pthread_t periodic_message_thread;
    pthread_create(&periodic_message_thread, NULL, send_periodic_message_to_active_servers, NULL);

    // Create socket
    if ((storage_server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Storage server socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&storage_server_addr, 0, sizeof(storage_server_addr));

    storage_server_addr.sin_family = AF_INET;
    storage_server_addr.sin_port = htons(SERVER_PORT);
    storage_server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(storage_server_socket, (struct sockaddr *)&storage_server_addr, sizeof(storage_server_addr)) == -1)
    {
        perror("Server socket bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(storage_server_socket, 10) == -1)
    {
        perror("Server socket listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Naming Server listening for storage servers on port %d...\n", SERVER_PORT);

    // Initialisation of n storage servers
    while (1)
    {
        storage_server_connection = accept(storage_server_socket, (struct sockaddr *)&storage_diff_addr, &storage_diff_addr_len);
        if (storage_server_connection < 0)
        {
            perror("Error in accepting server connection");
            continue;
        }

        printf("SS accepted\n");
        handle_server_requests(storage_server_connection);
        // close(storage_server_connection);
    }
    close(storage_server_socket);
    pthread_join(periodic_message_thread, NULL);
}

SS Send_SS_Details_To_Client(char *buffer, int client)
{
    char path[4096];
    char command[4096];
    char sendpath[4096];
    bzero(path, sizeof(path));
    bzero(command, sizeof(command));
    bzero(sendpath, sizeof(sendpath));
    SS ss_for_file;
    char status[4096];
    bzero(status, sizeof(status));
    sscanf(buffer, "%s %s", command, path);

    if (strncmp(path, "./", 2) == 0)
    {
        char *found = strstr(path, "./");
        if (found != NULL)
        {
            strncpy(sendpath, found + 2, sizeof(sendpath) - 1);
            sendpath[sizeof(sendpath) - 1] = '\0';
        }
        else
        {
            sendpath[0] = '\0';
        }
        ss_for_file = storage_server->next;
    }

    else
    {
        ss_for_file = search_helper(path, 0);
        strcpy(sendpath, path);
    }
    if (ss_for_file && ss_for_file->is_active == 0 && strcmp(command, "write") == 0)
    {
        SS ptr = storage_server;
        if (send(client, ptr, sizeof(struct SS_Info), 0) < 0)
        {
            strcpy(status, "Error in sending ss details\n");
            perror("Error in sending ss details");
        }
    }

    else if (ss_for_file)
    {
        if (ss_for_file->is_active == 0)
        {
            ss_for_file = search_backups(ss_for_file);
        }
        if (ss_for_file)
        {
            printf("Found in server: %d\n", ss_for_file->SS_index);
            strcpy(status, "Sent Successfully\n");
        }
        else
        {
            ss_for_file = storage_server;
            strcpy(status, "Not found\n");
        }
        if (send(client, ss_for_file, sizeof(struct SS_Info), 0) < 0)
        {
            strcpy(status, "Error in sending ss details\n");
            perror("Error in sending ss details");
        }

        if (strcmp(command, "write") == 0)
        {
            return ss_for_file;
        }
    }
    else
    {
        if (send(client, storage_server, sizeof(struct SS_Info), 0) < 0)
        {
            strcpy(status, "Error  in sending storage server details to client\n");
            perror("Error in sending storage server details to client");
        }

        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        getpeername(client, (struct sockaddr *)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Log the client request
        log_client_request(client_ip, ntohs(addr.sin_port), -1, buffer, status);
        return NULL;
    }
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    getpeername(client, (struct sockaddr *)&addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    // Log the client request
    log_client_request(client_ip, ntohs(addr.sin_port), ss_for_file->SS_index, buffer, status);
    return NULL;
}

void Ask_SS_To_Create_File(char *buffer, int client, int flag_for_feedback)
{
    char copy[4096];
    strcpy(copy, buffer);
    SS ss_for_file;

    char path[4096];
    char command[4096];
    char sendpath[4096];
    bzero(path, sizeof(path));
    bzero(command, sizeof(command));
    bzero(sendpath, sizeof(sendpath));

    sscanf(buffer, "%s %s", command, path);

    if (strncmp(path, "./", 2) == 0)
    {
        char *found = strstr(path, "./");
        if (found != NULL)
        {
            strncpy(sendpath, found + 2, sizeof(sendpath) - 1);
            sendpath[sizeof(sendpath) - 1] = '\0';
        }
        else
        {
            sendpath[0] = '\0';
        }
        ss_for_file = storage_server->next;
    }
    else
    {
        ss_for_file = search_helper(path, 1);
        strcpy(sendpath, path);
    }

    char status[128];
    bzero(status, sizeof(status));
    if (ss_for_file && ss_for_file->is_active != 0)
    {
        int storage_server_socket = ss_for_file->socket;
        printf("Found in ss %d\n", ss_for_file->SS_index);

        create_file(ss_for_file->SS_index, sendpath);
        send(storage_server_socket, copy, sizeof(copy), 0);
        recv(storage_server_socket, status, sizeof(status), 0);

        if (ss_for_file->backups[0])
        {
            send(ss_for_file->backups[0]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[0]->socket, status, sizeof(status), 0);
        }

        if (ss_for_file->backups[1])
        {
            send(ss_for_file->backups[1]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[1]->socket, status, sizeof(status), 0);
        }

        if (flag_for_feedback)
            send(client, status, sizeof(status), 0);
    }
    else
    {
        strcpy(status, "Failure");
        if (flag_for_feedback)
            send(client, status, sizeof(status), 0);
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        getpeername(client, (struct sockaddr *)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Log the client request
        log_client_request(client_ip, ntohs(addr.sin_port), -1, buffer, status);
        return;
    }
    if (flag_for_feedback)
    {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        getpeername(client, (struct sockaddr *)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Log the client request
        log_client_request(client_ip, ntohs(addr.sin_port), ss_for_file->SS_index, buffer, status);
        printf("%s is status\n", status);
    }
}

void Ask_SS_To_Create_Folder(char *buffer, int client, int flag_for_feedback)
{
    char copy[4096];
    bzero(copy, sizeof(copy));
    strcpy(copy, buffer);
    SS ss_for_file;

    char status[128];

    char path[4096];

    char command[4096];

    char sendpath[4096];
    bzero(sendpath, sizeof(sendpath));
    bzero(path, sizeof(path));
    bzero(command, sizeof(command));

    sscanf(buffer, "%s %s", command, path);
    if (path[strlen(path) - 1] != '/')
    {
        path[strlen(path)] = '/';
        path[strlen(path) + 1] = '\0';
    }
    if (strncmp(path, "./", 2) == 0)
    {
        char *found = strstr(path, "./");

        if (found != NULL)
        {
            strncpy(sendpath, found + 2, sizeof(sendpath) - 1);
            sendpath[sizeof(sendpath) - 1] = '\0';
        }
        else
        {
            sendpath[0] = '\0';
        }
        ss_for_file = storage_server->next;
    }
    else
    {
        ss_for_file = search_helper(path, 2);
        strcpy(sendpath, path);
    }

    if (ss_for_file != NULL && ss_for_file->is_active != 0)
    {
        if (sendpath[strlen(sendpath) - 1] != '/')
        {
            sendpath[strlen(sendpath)] = '/';
            sendpath[strlen(sendpath) + 1] = '\0';
        }
        int storage_server_socket = ss_for_file->socket;
        printf("Found in ss %d\n", ss_for_file->SS_index);

        create_folder(ss_for_file->SS_index, sendpath);
        send(storage_server_socket, copy, sizeof(copy), 0);
        recv(storage_server_socket, status, sizeof(status), 0);

        if (ss_for_file->backups[0] && ss_for_file->backups[0]->is_active)
        {
            send(ss_for_file->backups[0]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[0]->socket, status, sizeof(status), 0);
        }

        if (ss_for_file->backups[1] && ss_for_file->backups[1]->is_active)
        {
            send(ss_for_file->backups[1]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[1]->socket, status, sizeof(status), 0);
        }

        if (flag_for_feedback)
            send(client, status, sizeof(status), 0);
    }
    else
    {
        strcpy(status, "Not found");
        printf("%s is status\n", status);

        if (flag_for_feedback)
            send(client, status, sizeof(status), 0);
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        getpeername(client, (struct sockaddr *)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Log the client request
        log_client_request(client_ip, ntohs(addr.sin_port), -1, buffer, status);
        return;
    }
    if (flag_for_feedback)
    {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        getpeername(client, (struct sockaddr *)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Log the client request
        log_client_request(client_ip, ntohs(addr.sin_port), ss_for_file->SS_index, buffer, status);
        printf("%s is status\n", status);
    }
}

void Ask_SS_To_Delete_File(char *buffer, int client)
{
    char copy[4096];
    strcpy(copy, buffer);

    char status[16];
    char path[4096];
    char command[4096];
    bzero(status, sizeof(status));
    bzero(path, sizeof(path));
    bzero(command, sizeof(command));
    sscanf(buffer, "%s %s", command, path);
    SS ss_for_file = search_helper(path, 0);

    if (ss_for_file && ss_for_file->is_active != 0)
    {
        int storage_server_socket = ss_for_file->socket;
        printf("Found in ss %d\n", ss_for_file->SS_index);

        delete_file(path);
        send(storage_server_socket, copy, sizeof(copy), 0);
        recv(storage_server_socket, status, sizeof(status), 0);

        if (ss_for_file->backups[0])
        {
            send(ss_for_file->backups[0]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[0]->socket, status, sizeof(status), 0);
        }

        if (ss_for_file->backups[1])
        {
            send(ss_for_file->backups[1]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[1]->socket, status, sizeof(status), 0);
        }
        send(client, status, sizeof(status), 0);
    }
    else
    {
        strcpy(status, "Not found");
        send(client, status, sizeof(status), 0);

        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        getpeername(client, (struct sockaddr *)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Log the client request
        log_client_request(client_ip, ntohs(addr.sin_port), -1, buffer, status);
        return;
    }
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    getpeername(client, (struct sockaddr *)&addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    // Log the client request
    log_client_request(client_ip, ntohs(addr.sin_port), ss_for_file->SS_index, buffer, status);
    printf("%s is status\n", status);
}

void Ask_SS_To_Delete_Folder(char *buffer, int client)
{
    char copy[4096];
    strcpy(copy, buffer);

    char status[16];
    char path[4096];
    char command[4096];
    bzero(status, sizeof(status));
    bzero(path, sizeof(path));
    bzero(command, sizeof(command));
    sscanf(buffer, "%s %s", command, path);
    SS ss_for_file = search_helper(path, 0);

    if (ss_for_file && ss_for_file->is_active != 0)
    {
        printf("Found in ss %d\n", ss_for_file->SS_index);
        delete_folder(path);
        int storage_server_socket = ss_for_file->socket;
        send(storage_server_socket, copy, sizeof(copy), 0);
        recv(storage_server_socket, status, sizeof(status), 0);

        if (ss_for_file->backups[0])
        {
            send(ss_for_file->backups[0]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[0]->socket, status, sizeof(status), 0);
        }

        if (ss_for_file->backups[1])
        {
            send(ss_for_file->backups[1]->socket, copy, sizeof(copy), 0);
            recv(ss_for_file->backups[1]->socket, status, sizeof(status), 0);
        }
        send(client, status, sizeof(status), 0);
    }
    else
    {
        strcpy(status, "Failure");
        send(client, status, sizeof(status), 0);

        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        getpeername(client, (struct sockaddr *)&addr, &addr_len);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        // Log the client request
        log_client_request(client_ip, ntohs(addr.sin_port), -1, buffer, status);
        return;
    }
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    getpeername(client, (struct sockaddr *)&addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    // Log the client request
    log_client_request(client_ip, ntohs(addr.sin_port), ss_for_file->SS_index, buffer, status);
    printf("%s is status\n", status);
}

char *extract_filename(char *src_dir)
{
    char copy[4096];
    bzero(copy, sizeof(copy));
    strcpy(copy, src_dir);
    char *token = strtok(copy, "/");
    char *filename = (char *)malloc(sizeof(char) * 1024);
    while (token != NULL)
    {
        strcpy(filename, token);
        token = strtok(NULL, "/");
    }
    return filename;
}

SS Copy_File(char *buffer, int client, int flag_for_feedback)
{
    char status1[256];
    bzero(status1, sizeof(status1));

    char command[4096];
    bzero(command, sizeof(command));

    strcpy(command, buffer);

    // Identify source and destination
    char todo[32];
    char filename[100];
    char dest_path[256];

    bzero(todo, sizeof(todo));
    bzero(filename, sizeof(filename));
    bzero(dest_path, sizeof(dest_path));

    sscanf(buffer, "%s %s %s", todo, filename, dest_path);

    if (dest_path[strlen(dest_path) - 1] != '/')
    {
        dest_path[strlen(dest_path)] = '/';
        dest_path[strlen(dest_path) + 1] = '\0';
    }

    // Find which SS they are located in
    SS ss_for_file = search_helper(filename, 0);
    SS ss_for_dest = search_helper(dest_path, 0);

    if (ss_for_file && ss_for_dest && ss_for_dest->is_active == 1)
    {
        if (ss_for_file->is_active == 0)
        {
            ss_for_file = search_backups(ss_for_file);
        }

        int storage_server_socket_src = ss_for_file->socket;
        int storage_server_socket_dest = ss_for_dest->socket;
        char make_create_instr[4096] = "create_file ";

        char newfile_path[4096];
        bzero(newfile_path, sizeof(newfile_path));

        strcpy(newfile_path, dest_path);

        char *file = extract_filename(filename);
        strcat(newfile_path, file);

        strcat(make_create_instr, newfile_path);

        // printf("%s..\n", make_create_instr);

        Ask_SS_To_Create_File(make_create_instr, client, 0);

        char make_read_instr[4096] = "read ";
        strcat(make_read_instr, filename);

        send(storage_server_socket_src, make_read_instr, sizeof(make_read_instr), 0);

        strcpy(status1, Handle_Read_Request(storage_server_socket_src, storage_server_socket_dest, newfile_path));

        // printf("Read recieve:%s\n", file_contents);

        // Create new file in destination_SS

        // Write to this file in destination_SS
        // char make_write_instr[256] = "write ";
        // strcat(make_write_instr, newfile_path);

        // strcat(make_write_instr, " ");

        // strcat(make_write_instr, file_contents);

        // send(storage_server_socket_dest, make_write_instr, sizeof(make_write_instr), 0);

        // char status1[256]; // from write
        // // char status2[256];  // from
        // recv(storage_server_socket_dest, status1, sizeof(status1), 0); // from handle_write_request in SS

        // Update the accessible paths of destination_SS

        // Send status of priviledged operation to client
        if (flag_for_feedback)
        {
            send(client, status1, sizeof(status1), 0);
            if (strcmp(status1, "Success") != 0)
            {
                ss_for_file = NULL;
            }
        }
    }
    else if (flag_for_feedback)
    {
        strcpy(status1, "Not found");
        if (send(client, status1, sizeof(status1), 0) < 0)
        {
            perror("Error in sending status from nm to client:");
        }
        ss_for_file = NULL;
    }
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    getpeername(client, (struct sockaddr *)&addr, &addr_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int index;
    // Log the client request
    if (ss_for_file == NULL)
    {
        index = -1;
    }
    else
    {
        index = ss_for_file->SS_index;
    }

    log_client_request(client_ip, ntohs(addr.sin_port), index, buffer, status1);
    return ss_for_file;
}

char *create_folders(char *buffer, int client, char *dir_made_till_now)
{
    char *tok = strtok(buffer, "/");
    char make_create_command[4096] = "create_folder ";

    char cachee[4096];
    bzero(cachee, sizeof(cachee));

    while (tok != NULL)
    {
        strcpy(cachee, tok);
        tok = strtok(NULL, "/");
    }

    strcat(dir_made_till_now, cachee);
    strcat(make_create_command, dir_made_till_now);
    printf("%s..\n", make_create_command);
    Ask_SS_To_Create_Folder(make_create_command, client, 0);

    strcat(dir_made_till_now, "/");
    return dir_made_till_now;
}

char *find_common(char *str1, char *str2)
{
    char *common = (char *)malloc(sizeof(char) * 128);
    bzero(common, sizeof(common));

    int src_len = strlen(str1);
    int dest_len = strlen(str2);

    int min = 0;
    if (src_len > dest_len)
    {
        min = src_len;
    }
    else
    {
        min = dest_len;
    }

    int i;
    for (i = 0; i < min; i++)
    {
        if (str1[i] == str2[i])
        {
            common[i] = str1[i];
        }
        else
            break;
    }

    for (int k = i - 1; k >= 0; k--)
    {
        if (common[k] != '/')
            common[k] = '\0';
        else
            break;
    }
    return common;
}

char *create_folders_inner(char *buffer, char *dir_made_till_now)
{
    char copy[4096];
    bzero(copy, sizeof(copy));
    strcpy(copy, buffer);

    char *made = (char *)malloc(sizeof(char) * 1024);
    strcpy(made, "");

    char *made_dup = (char *)malloc(sizeof(char) * 1024);
    strcpy(made_dup, "");

    char *tok = strtok(copy, "/");

    while (tok != NULL)
    {
        // printf("%s\n", tok);
        strcat(made_dup, tok);
        char *found = strstr(dir_made_till_now, made_dup);
        if (found == NULL)
        {
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

void extractSubstring(const char *source, char *destination)
{
    const char *lastSlash = strrchr(source, '/');

    if (lastSlash != NULL)
    {
        // Calculate the length of the substring
        int length = lastSlash - source + 1;

        // Copy the substring until the last slash
        strncpy(destination, source, length);
        destination[length] = '\0'; // Null-terminate the destination string
    }
    else
    {
        // If no slash is found, copy the entire string
        strcpy(destination, source);
    }
}

SS Copy_Folder(char *buffer, int client)
{
    char status[32];
    bzero(status, sizeof(status));
    char todo[16];
    bzero(todo, sizeof(todo));

    char src_dir[4096];
    bzero(src_dir, sizeof(src_dir));

    char dest_dir[4096];
    bzero(dest_dir, sizeof(dest_dir));

    char copy[4096];
    bzero(copy, sizeof(copy));

    strcpy(copy, buffer);

    // Identify source and destination directory
    sscanf(buffer, "%s %s %s", todo, src_dir, dest_dir);

    if (src_dir[strlen(src_dir) - 1] != '/')
    {
        src_dir[strlen(src_dir)] = '/';
        src_dir[strlen(src_dir) + 1] = '\0';
    }

    if (dest_dir[strlen(dest_dir) - 1] != '/')
    {
        dest_dir[strlen(dest_dir)] = '/';
        dest_dir[strlen(dest_dir) + 1] = '\0';
    }

    // Look for the SS containing this dir
    SS ss_for_folder = search_helper(src_dir, 0);
    SS ss_for_dest = search_helper(dest_dir, 0);

    if (ss_for_folder && ss_for_dest && ss_for_dest->is_active == 1)
    {
        if (ss_for_folder->is_active == 0)
        {
            ss_for_folder = search_backups(ss_for_folder);
        }
        int n;
        char **s = subpaths_in_trie(&n, src_dir);

        char common[4096];
        bzero(common, sizeof(common));

        int src_len = strlen(src_dir);
        int dest_len = strlen(dest_dir);

        int min = 0;
        if (src_len > dest_len)
        {
            min = src_len;
        }
        else
        {
            min = dest_len;
        }

        int i;
        for (i = 0; i < min; i++)
        {
            if (src_dir[i] == dest_dir[i])
            {
                common[i] = src_dir[i];
            }
            else
                break;
        }

        for (int k = i - 1; k >= 0; k--)
        {
            if (common[k] != '/')
                common[k] = '\0';
            else
                break;
        }

        char *dir_made_till_now = (char *)malloc(sizeof(char) * 4096);
        bzero(dir_made_till_now, sizeof(dir_made_till_now));

        char dir_made_till_now_mandatory[4096];
        bzero(dir_made_till_now_mandatory, sizeof(dir_made_till_now_mandatory));

        strcpy(dir_made_till_now, dest_dir);

        char src_dir_copy[4096];
        bzero(src_dir_copy, sizeof(src_dir_copy));

        strcpy(src_dir_copy, src_dir);

        if (strcmp(common, "") == 0)
        {
            strcpy(dir_made_till_now_mandatory, create_folders(src_dir_copy, client, dir_made_till_now));
            strcpy(dir_made_till_now, dir_made_till_now_mandatory);
        }

        else
        {
            strcpy(src_dir_copy, src_dir + strlen(common));
            strcpy(dir_made_till_now_mandatory, create_folders(src_dir_copy, client, dir_made_till_now));
            strcpy(dir_made_till_now, dir_made_till_now_mandatory);
        }

        char without_src_dir[n][4096];
        for (int i = 0; i < n; i++)
        {
            bzero(without_src_dir[i], sizeof(without_src_dir[i]));
        }

        for (int i = 1; i < n; i++)
        {
            strcpy(without_src_dir[i], s[i] + strlen(src_dir));

            char *found = strstr(without_src_dir[i], "/");
            if (found == NULL)
            {
                char make_command[4096] = "copy_file ";
                strcat(make_command, s[i]);
                strcat(make_command, " ");
                strcat(make_command, dir_made_till_now_mandatory);

                if (make_command[strlen(make_command) - 1] == '/')
                {
                    make_command[strlen(make_command) - 1] = '\0';
                }
                else
                {
                    make_command[strlen(make_command)] = '\0';
                }
                // printf("%s\n", make_command);
                Copy_File(make_command, client, 0);
            }
            else
            {
                // handle dird1/dirc1
                char *x = create_folders_inner(without_src_dir[i], dir_made_till_now);
                if (strcmp(x, "") == 0)
                {
                    char make_command[4096] = "create_folder ";
                    strcat(make_command, dir_made_till_now_mandatory);
                    strcat(make_command, without_src_dir[i]);

                    strcpy(dir_made_till_now, dir_made_till_now_mandatory);
                    strcat(dir_made_till_now, without_src_dir[i]);

                    if (make_command[strlen(make_command) - 1] == '/')
                        make_command[strlen(make_command) - 1] = '\0';

                    printf("%s\n", make_command);

                    Ask_SS_To_Create_Folder(make_command, client, 0);
                }
                else
                {
                    char not_made[4096];
                    bzero(not_made, sizeof(not_made));

                    strcpy(not_made, without_src_dir[i] + strlen(x));
                    if (strstr(not_made, "/") != NULL)
                    {
                        char make_command[256] = "create_folder ";

                        strcat(make_command, dir_made_till_now);
                        strcat(make_command, not_made);

                        strcat(dir_made_till_now, not_made);
                        make_command[strlen(make_command) - 1] = '\0';

                        // printf("%s\n", make_command);
                        Ask_SS_To_Create_Folder(make_command, client, 0);
                    }
                    else
                    {
                        char make_command[4096] = "copy_file ";
                        strcat(make_command, s[i]);

                        char *till_last_slash = (char *)malloc(sizeof(char) * 1024);
                        extractSubstring(s[i], till_last_slash);

                        char *found_dir = strstr(dir_made_till_now, till_last_slash);
                        if (found_dir)
                        {
                            int start = found_dir - dir_made_till_now;
                            int end = start + strlen(till_last_slash) - 1;

                            int j_end = strlen(dir_made_till_now);
                            for (int j = end + 1; j < j_end; j++)
                            {
                                dir_made_till_now[j] = '\0';
                            }
                        }

                        strcat(make_command, " ");
                        strcat(make_command, dir_made_till_now);

                        if (make_command[strlen(make_command) - 1] == '/')
                            make_command[strlen(make_command) - 1] = '\0';

                        // printf("%s\n", make_command);
                        Copy_File(make_command, client, 0);
                    }
                }
            }
        }
        strcpy(status, "Sucess");
        send(client, status, sizeof(status), 0);
        return ss_for_folder;
    }
    else
    {
        strcpy(status, "Not found");
        send(client, status, sizeof(status), 0);

        return NULL;
    }
}

void *asynchronous_copy_file(void *arg)
{
    // struct command_ss ss_and_comm = *(struct command_ss *)arg;

    // if (ss_and_comm.ss->backups[0] != NULL && ss_and_comm.ss->backups[0]->is_active != 0)
    // {
    //     Copy_File_redundancy(ss_and_comm.command, ss_and_comm.ss, ss_and_comm.ss->backups[0]);
    // }
    // if (ss_and_comm.ss->backups[1] != NULL && ss_and_comm.ss->backups[1]->is_active != 0)
    // {
    //     Copy_File_redundancy(ss_and_comm.command, ss_and_comm.ss, ss_and_comm.ss->backups[1]);
    // }

    return NULL;
}
void *asynchronous_copy_folder(void *arg)
{
    // struct command_ss ss_and_comm = *(struct command_ss *)arg;
    // char todo[16];
    // bzero(todo, sizeof(todo));

    // char src_dir[4096];
    // bzero(src_dir, sizeof(src_dir));

    // char dest_dir[4096];
    // bzero(dest_dir, sizeof(dest_dir));

    // // Identify source and destination directory
    // sscanf(ss_and_comm.command, "%s %s %s", todo, src_dir, dest_dir);
    // if (ss_and_comm.ss->backups[0] != NULL && ss_and_comm.ss->backups[0]->is_active != 0)
    // {
    //     Copy_Folder_redundancy(src_dir, ss_and_comm.ss, ss_and_comm.ss->backups[0]);
    // }
    // if (ss_and_comm.ss->backups[1] != NULL && ss_and_comm.ss->backups[1]->is_active != 0)
    // {
    //     Copy_Folder_redundancy(src_dir, ss_and_comm.ss, ss_and_comm.ss->backups[1]);
    // }

    return NULL;
}

void *asynchronous_write(void *arg)
{
    struct command_ss ss_and_comm = *(struct command_ss *)arg;

    char sendpath[4096];
    bzero(sendpath, sizeof(sendpath));
    char comm[256];
    bzero(comm, sizeof(comm));
    sscanf(ss_and_comm.command, "%s %s", comm, sendpath);

    char message[256];
    bzero(message, sizeof(message));
    int bytes_rec = recv(ss_and_comm.cl, message, sizeof(message), 0);
    if (bytes_rec < 0)
    {
        perror("Error in receiving message from client");
        return NULL;
    }
    if (strcmp(message, "done") == 0)
    {
        char command_2[4096];
        bzero(command_2, sizeof(command_2));
        strcpy(command_2, "copy_file ");
        strcat(command_2, sendpath);
        strcat(command_2, " ");
        char *lastSlash = strrchr(sendpath, '/');
        int lengthUpToLastSlash = lastSlash - sendpath + 1;
        char destdir[4096];
        bzero(destdir, sizeof(destdir));
        strncpy(destdir, sendpath, lengthUpToLastSlash);
        strcat(command_2, destdir);
        if (ss_and_comm.ss->backups[0] != NULL && ss_and_comm.ss->backups[0]->is_active != 0)
        {
            Copy_File_redundancy(command_2, ss_and_comm.ss, ss_and_comm.ss->backups[0]);
        }
        if (ss_and_comm.ss->backups[1] != NULL && ss_and_comm.ss->backups[1]->is_active != 0)
        {
            Copy_File_redundancy(command_2, ss_and_comm.ss, ss_and_comm.ss->backups[1]);
        }
    }
    return NULL;
}

void handle_client_requests(char *buffer, int client)
{
    char copy[4096];
    strcpy(copy, buffer);

    if (strncmp(buffer, "read", 4) == 0 || strncmp(buffer, "write", 5) == 0 || strncmp(buffer, "get_size_per", 12) == 0)
    {
        SS ss_for_file = Send_SS_Details_To_Client(buffer, client);
        if (strncmp(buffer, "write", 5) == 0 && ss_for_file != NULL && ss_for_file->is_active != 0)
        {
            struct command_ss comm_ss;
            comm_ss.cl = client;
            comm_ss.ss = ss_for_file;
            strcpy(comm_ss.command, buffer);
            pthread_t async_write;
            pthread_create(&async_write, NULL, asynchronous_write, &comm_ss);
            pthread_join(async_write, NULL);
        }
    }
    else
    {
        if (strncmp(buffer, "create_file", 11) == 0)
        {
            Ask_SS_To_Create_File(buffer, client, 1);
        }
        else if (strncmp(buffer, "create_folder", 13) == 0)
        {
            Ask_SS_To_Create_Folder(buffer, client, 1);
        }
        else if (strncmp(buffer, "delete_file", 11) == 0)
        {
            Ask_SS_To_Delete_File(buffer, client);
        }
        else if (strncmp(buffer, "delete_folder", 13) == 0)
        {
            Ask_SS_To_Delete_Folder(buffer, client);
        }
        else if (strncmp(buffer, "copy_file", 9) == 0)
        {

            SS ss_for_file = Copy_File(buffer, client, 1);
            if (ss_for_file != NULL && ss_for_file->is_active != 0)
            {
                struct command_ss comm_ss;
                comm_ss.cl = client;
                comm_ss.ss = ss_for_file;
                strcpy(comm_ss.command, buffer);
                pthread_t async_copy;
                pthread_create(&async_copy, NULL, asynchronous_copy_file, &comm_ss);
                pthread_join(async_copy, NULL);
            }
        }
        else if (strncmp(buffer, "copy_folder", 11) == 0)
        {
            SS ss_for_file = Copy_Folder(buffer, client);

            if (ss_for_file != NULL && ss_for_file->is_active != 0)
            {
                struct command_ss comm_ss;
                comm_ss.cl = client;
                comm_ss.ss = ss_for_file;
                strcpy(comm_ss.command, buffer);
                pthread_t async_copy_f;
                pthread_create(&async_copy_f, NULL, asynchronous_copy_folder, &comm_ss);
                pthread_join(async_copy_f, NULL);
            }
        }
    }
}

void *handle_clients_specific(void *client_port)
{
    int port = *(int *)client_port;
    int client_socket, client_connection;
    struct sockaddr_in client_addr, client_diff_addr;
    socklen_t client_diff_addr_len;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        HandleErrors(5);
        exit(EXIT_FAILURE);
    }

    memset(&client_addr, 0, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    client_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind

    if (bind(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
    {
        HandleErrors(5);
        exit(EXIT_FAILURE);
    }

    if (listen(client_socket, 10) == -1)
    {
        HandleErrors(5);
        exit(EXIT_FAILURE);
    }
    printf("Naming Server listening for clients on port %d...\n", port);

    while (1)
    {
        client_connection = accept(client_socket, (struct sockaddr *)&client_diff_addr, &client_diff_addr_len);

        if (client_connection < 0)
        {
            HandleErrors(5);
            continue;
        }

        printf(PINK "Naming server accepted client on port %d" WHITE "\n", port);

        char buffer[4096];
        bzero(buffer, sizeof(buffer));

        while (1)
        {
            int bytes_rec = recv(client_connection, buffer, sizeof(buffer), 0);
            if (bytes_rec < 0)
            {
                HandleErrors(7);
                continue;
            }
            else if (bytes_rec == 0)
            {
                continue;
            }
            buffer[strlen(buffer)] = '\0';

            printf(PINK "Got request: %s" WHITE "\n", buffer);

            char ack_bit[4096] = "Recieved your request!!";

            if (send(client_connection, ack_bit, sizeof(ack_bit), 0) < 0)
            {
                HandleErrors(7);
            }

            if (strncmp(buffer, "exit", 4) == 0)
            {
                break;
            }
            else
            {
                handle_client_requests(buffer, client_connection);
            }
        }
    }
    close(client_socket);
}

void *handle_clients()
{
    int client_socket, client_connection;
    struct sockaddr_in client_addr, client_diff_addr;
    socklen_t client_diff_addr_len;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        HandleErrors(5);
        exit(EXIT_FAILURE);
    }

    memset(&client_addr, 0, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind

    if (bind(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
    {
        HandleErrors(5);
        exit(EXIT_FAILURE);
    }

    if (listen(client_socket, 10) == -1)
    {
        HandleErrors(5);
        exit(EXIT_FAILURE);
    }
    printf(PINK "Naming Server listening for clients on port %d..." WHITE "\n", CLIENT_PORT);

    while (1)
    {
        client_connection = accept(client_socket, (struct sockaddr *)&client_diff_addr, &client_diff_addr_len);

        if (client_connection < 0)
        {
            HandleErrors(5);
            continue;
        }

        printf(PINK "Naming server accepted client. Looking for specific port" WHITE "\n");

        // Now check for any free port and send client to that port
        int specific_client_port_index = return_free_port();
        if (specific_client_port_index == -1)
        {
            HandleErrors(5);
            pthread_exit(NULL);
        }

        int specific_client_port = all_ports[specific_client_port_index].port;
        all_ports[specific_client_port_index].free = 0;

        pthread_create(&all_client_threads[client_index++], NULL, handle_clients_specific, &specific_client_port);

        if (send(client_connection, &specific_client_port, sizeof(int), 0) < 0)
        {
            HandleErrors(7);
        }
    }

    for (int i = 0; i < client_index; i++)
    {
        pthread_join(all_client_threads[i], NULL);
    }
}

int main()
{
    initialise_all_ports_array();

    pthread_t storage_server_thread, client_thread;
    pthread_create(&storage_server_thread, NULL, handle_storage_servers, NULL);
    pthread_create(&client_thread, NULL, handle_clients, NULL);

    pthread_join(client_thread, NULL);
    pthread_join(storage_server_thread, NULL);
}