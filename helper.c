#include "ns.h"

SS find_ss_by_port(int port_no)
{
    SS ptr = storage_server->next;
    SS return_this = NULL;
    while (ptr != NULL)
    {
        if (ptr->SS_Port == port_no)
        {
            return_this = ptr;
            return return_this;
        }

        ptr = ptr->next;
    }
    return 0;
}

SS find_ss_by_index(int ss_index)
{
    SS ptr = storage_server->next;
    SS return_this = NULL;
    while (ptr != NULL)
    {
        if (ptr->SS_index == ss_index)
        {
            return_this = ptr;
            return return_this;
        }

        ptr = ptr->next;
    }
    return NULL;
}

SS Make_Node_SS(char *buffer, int server)
{
    char all_variables[4][4096];
    // char accessible_paths[4096];
    int index = 0;
    char *token = strtok(buffer, "\n");
    token = strtok(NULL, "\n");

    while (token != NULL)
    {
        strcpy(all_variables[index++], token);
        token = strtok(NULL, "\n");
    }
    int ss_port = atoi(all_variables[2]);
    SS prev_serv = find_ss_by_port(ss_port);
    if (prev_serv)
    {
        prev_serv->socket = server;
        char **all_ss_paths = (char **)malloc(sizeof(char) * MAX_PATHS);

        for (int i = 0; i < MAX_PATHS; i++)
        {
            all_ss_paths[i] = (char *)malloc(sizeof(char) * 2048);
            bzero(all_ss_paths[i], sizeof(all_ss_paths[i]));
        }

        int n;

        get_all_path_depth(accessible_paths_trie, "", &n, all_ss_paths, prev_serv->SS_index);

        char all_paths[MAX_SUM_OF_ALL_PATHS] = "";
        // bzero(all_paths,sizeof(all_paths));

        // printf("%d\n",n);

        for (int i = 0; i < n; i++)
        {
            strcat(all_paths, all_ss_paths[i]);
            strcat(all_paths, "\n");
        }

        send(server, all_paths, sizeof(all_paths), 0);
        return NULL;
    }

    SS newnode = (SS)malloc(sizeof(struct SS_Info));
    newnode->socket = server;
    newnode->SS_index = storage_server_tail->SS_index + 1;
    newnode->NM_Port = atoi(all_variables[1]);
    newnode->SS_Port = atoi(all_variables[2]);
    newnode->is_active = 1;
    strcpy(newnode->IP_addr, all_variables[0]);

    int ss_index = newnode->SS_index;
    if (ss_index > 3)
    {
        newnode->backups[0] = storage_server_tail;
        newnode->backups[1] = find_ss_by_index(ss_index - 2);
    }

    char *tok = strtok(all_variables[3], " ");
    while (tok != NULL)
    {
        insert(tok, strlen(tok), ss_index);
        tok = strtok(NULL, " ");
    }

    newnode->next = NULL;
    return newnode;
}

int Make_Socket_For_SS(SS storage_server_details)
{
    int ss_socket;
    struct sockaddr_in addr;

    ss_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_socket < 0)
    {
        perror("Error in creating socket for communicating with storage server");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(storage_server_details->SS_Port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(ss_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Error in connecting to storage server");
        exit(EXIT_FAILURE);
    }

    return ss_socket;
}

void send_message_to_active_servers()
{
    SS current_server = storage_server->next;
    while (current_server != NULL)
    {
        int ss_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (ss_socket == -1)
        {
            perror("Socket creation failed");
            return;
        }
        // Define the server address structure
        struct sockaddr_in ss_addr;
        ss_addr.sin_family = AF_INET;
        ss_addr.sin_port = htons(current_server->SS_Port);
        ss_addr.sin_addr.s_addr = INADDR_ANY;

        // Connect to the server
        if (connect(ss_socket, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) == -1)
        {
            if (current_server->is_active == 1)
            {
                printf("Storage server %d down\n", current_server->SS_index);
            }
            current_server->is_active = 0;
        }

        else
        {
            char nmmessg[32] = "nmmssg";

            send(ss_socket, nmmessg, sizeof(nmmessg), 0);
            if (current_server->is_active == 0)
            {
                printf("Storage server %d back online\n", current_server->SS_index);
            }
            current_server->is_active = 1;
        }
        close(ss_socket);
        current_server = current_server->next;
    }
}
char *Handle_Read_Request(int ss_socket, int storage_server_socket_dest, char *newfile_path)
{
    char receiveBuffer[CHUNK_SIZE];
    // strcpy(receiveBuffer, "");
    // bzero(receiveBuffer, sizeof(receiveBuffer));
    int bytesRead = 0;
    int check;
    // size_t totalBytesReceived = 0;
    long toget;
    char *status = (char *)malloc(sizeof(char) * 256);
    bzero(status, sizeof(status));
    recv(ss_socket, &toget, sizeof(long), 0);
    // char *file_contents = (char *)malloc(sizeof(char) * (toget + 1));

    // strcpy(file_contents, "");
    while (bytesRead < toget)
    {
        bzero(receiveBuffer, sizeof(receiveBuffer));
        check = recv(ss_socket, receiveBuffer, CHUNK_SIZE, 0);
        if (check > 0)
        {
            bytesRead += check;
        }

        else if (check == 0)
        {
            continue;
        }
        receiveBuffer[CHUNK_SIZE] = '\0';
        // strcat(file_contents, receiveBuffer);
        char make_write_instr[4096];
        bzero(make_write_instr, sizeof(make_write_instr));
        strcpy(make_write_instr, "write ");

        strcat(make_write_instr, newfile_path);

        strcat(make_write_instr, " ");

        strcat(make_write_instr, receiveBuffer);
        send(storage_server_socket_dest, make_write_instr, sizeof(make_write_instr), 0);

        recv(storage_server_socket_dest, status, sizeof(status), 0); // from handle_write_request in SS
        // Updat
    }
    return status;
}

void *send_periodic_message_to_active_servers()
{
    while (1)
    {
        send_message_to_active_servers();
        sleep(10);
    }
}

void log_client_request(char *client_ip, int client_port, int ss_index, char *request, char *acknowledgement)
{
    FILE *log_file;

    log_file = fopen("bookkeeping_log.txt", "a");

    if (log_file == NULL)
    {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    fprintf(log_file, "Client IP: %s\n", client_ip);
    fprintf(log_file, "Client Port: %d\n", client_port);
    fprintf(log_file, "Storage Server Index: %d\n", ss_index);
    fprintf(log_file, "Request: %s\n", request);
    fprintf(log_file, "Acknowledgement: %s\n", acknowledgement);
    fprintf(log_file, "---------------------------\n");

    fclose(log_file);
}