#include "client.h"
#include "defs.h"

#define NM_PORT 8080
#define SS_PORT 5555
#define SERVER_IP "127.0.0.1"

#define CONTENT_SIZE 100
#define CHUNK_SIZE 100

#define TIMEOUT_SEC 5

int Socket_For_Client_To_NM(int port)
{
    int nm_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_socket == -1)
    {
        perror("Socket creation failed");
        return -1;
    }

    // Define the server address structures
    struct sockaddr_in nm_addr;
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(port);
    nm_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server
    if (connect(nm_socket, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) == -1)
    {
        perror("Connection to the naming server failed");
        return -1;
    }

    return nm_socket;
}

int Socket_For_Client_To_SS(int ss_port)
{
    int ss_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_socket == -1)
    {
        perror("Socket creation failed");
        return -1;
    }

    // Define the server address structure
    struct sockaddr_in ss_addr;
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_port = htons(ss_port);
    ss_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server
    if (connect(ss_socket, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) == -1)
    {
        perror("Connection to the storage server failed");
        close(ss_socket);
        return -1;
    }

    return ss_socket;
}

char *Handle_Read_Request(int ss_socket)
{
    char receiveBuffer[CHUNK_SIZE + 1];
    bzero(receiveBuffer, sizeof(receiveBuffer));
    strcpy(receiveBuffer, "");
    int bytesRead = 0;
    int check;
    // size_t totalBytesReceived = 0;
    long toget;

    recv(ss_socket, &toget, sizeof(long), 0);

    char *file_contents = (char *)malloc(sizeof(char) * (toget + 1));

    strcpy(file_contents, "");
    while (bytesRead < toget)
    {
        bzero(receiveBuffer, sizeof(receiveBuffer));
        check = recv(ss_socket, receiveBuffer, sizeof(receiveBuffer), 0);

        if (check > 0)
        {
            bytesRead += check;
        }
        // strcat(file_contents, receiveBuffer);
        printf("%s", receiveBuffer);
    }
    return file_contents;
}

void Handle_Write_Request(int ss_socket, char *request)
{
    char write_allowed[4096];
    bzero(write_allowed, sizeof(write_allowed));

    recv(ss_socket, write_allowed, sizeof(write_allowed), 0);

    if (strcmp(write_allowed, "Ready") == 0)
    {
        char copy[256];
        strcpy(copy, request);

        char filename[4096];

        char *token = strtok(copy, " ");
        strcpy(filename, strtok(NULL, " "));

        (void)token;

        char contents[CONTENT_SIZE + 1] = "";

        while (strncmp(contents, "STOP", 4) != 0)
        {
            printf(YELLOW "Enter contents to write to %s: " WHITE, filename);
            fgets(contents, sizeof(contents), stdin);
            send(ss_socket, contents, sizeof(contents), 0);
        }
        char status[16];
        recv(ss_socket, status, sizeof(status), 0);
        printf(PINK "Contents written to file: %s\n" WHITE, status);
    }
    else
    {
        printf("%s\n", write_allowed);
        return;
    }
}

void Handle_Get_Size_Per(int ss_socket)
{
    struct size_per *info = (struct size_per *)malloc(sizeof(struct size_per));
    recv(ss_socket, info, sizeof(struct size_per), 0);
    printf("File permissions: %o\n", info->file_permissions);
    printf("File size: %ld\n", info->file_size);

    char ack[5];
    while (1)
    {
        recv(ss_socket, ack, sizeof(ack), 0);
        if (strcmp(ack, "") != 0)
            break;
    }
    printf("%s\n", ack);
}

void Process_Read_Write_Get_Request(int sock, char *todo)
{
    char request[256];
    bzero(request, sizeof(request));
    strcpy(request, todo);

    SS storage_server_details = (SS)malloc(sizeof(struct SS_Info));
    if (recv(sock, storage_server_details, sizeof(struct SS_Info), 0) < 0)
    {
        perror("Error in recieving storage server details from naming server");
        return;
    }
    if (storage_server_details->SS_index <= 0)
    {
        printf(RED "Not found\n" WHITE);

        return;
    }

    int ss_socket = Socket_For_Client_To_SS(storage_server_details->SS_Port);

    printf("%d\n", storage_server_details->SS_Port);

    if (send(ss_socket, request, sizeof(request), 0) < 0)
    {
        perror("Error in sending request to storage server from client");
        return;
    }

    if (strncmp(request, "get_size_per", 12) == 0)
    {
        Handle_Get_Size_Per(ss_socket);
        // struct size_per *info = (struct size_per *)malloc(sizeof(struct size_per));
        // recv(ss_socket, info, sizeof(struct size_per), 0);
        // printf("File permissions: %o\n", info->file_permissions);
        // printf("File size: %ld\n", info->file_size);
    }

    else if (strncmp(request, "read", 4) == 0)
    {
        Handle_Read_Request(ss_socket);
        // printf("%s\n", file_contents);
        printf(YELLOW "All contents recieved: Success\n" WHITE);
        close(ss_socket);
    }

    else if (strncmp(request, "write", 5) == 0)
    {
        Handle_Write_Request(ss_socket, todo);
        char message[16] = "done";
        if (send(sock, message, sizeof(message), 0) < 0)
        {
            perror("Error in sending status to NM");
            return;
        }
    }
}

int Send_Request_To_NM(int sock, char *msg)
{
    char request[4096];
    bzero(request, sizeof(request));

    strcpy(request, msg);

    if (send(sock, request, sizeof(request), 0) < 0)
    {
        perror("Error in sending client request to naming server");
        return -1;
    }
    return 1;
}

void Get_And_Print_Status_From_NM(int sock)
{
    char status[256];
    if (recv(sock, status, sizeof(status), 0) < 0)
    {
        perror("Error in recieving status from naming server");
        exit(EXIT_FAILURE);
    }

    printf(YELLOW "%s\n" WHITE, status);
}

int main()
{
    int sock_common = Socket_For_Client_To_NM(NM_PORT);

    int specific_port;

    recv(sock_common, &specific_port, sizeof(int), 0);

    close(sock_common);

    printf("Recieved port %d for communicating\n", specific_port);

    int sock = Socket_For_Client_To_NM(specific_port);

    char request[4096];

    printf("-----Operations------\n");
    printf("1. create_file <path>\n");
    printf("2. create_folder <path>\n");
    printf("3. delete_file <path>\n");
    printf("4. delete_folder <path>\n");
    printf("5. copy_file <path_from> <path_to>\n");
    printf("6. copy_folder <path_from> <path_to>\n");
    printf("7. read <path>\n");
    printf("8. write <path>\n");
    printf("9. get_size_per <path>\n");
    printf("10. exit\n");
    printf("--------------------\n");
    while (1)
    {
        bzero(request, sizeof(request));
        printf("Enter your request: ");

        fgets(request, sizeof(request), stdin);
        request[strlen(request) - 1] = '\0';

        if (Send_Request_To_NM(sock, request) == -1)
        {
            continue;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;

        int ready = select(sock + 1, &readfds, NULL, NULL, &timeout);

        if (ready == -1)
        {
            perror("select");
        }

        else if (ready == 0)
        {
            // Timeout occurred
            printf("Timeout occurred. Server did not respond within 5 seconds.\n");
            break;
        }

        else
        {
            if (FD_ISSET(sock, &readfds))
            {
                char ack_bit[4096];
                bzero(ack_bit, sizeof(ack_bit));
                recv(sock, ack_bit, sizeof(ack_bit), 0);

                printf("From naming server: %s\n", ack_bit);

                // status of privileged operations
                if (strncmp(request, "create", 6) == 0 || strncmp(request, "delete", 6) == 0)
                {
                    Get_And_Print_Status_From_NM(sock);
                    continue;
                }
                else if (strncmp(request, "copy", 4) == 0)
                {
                    Get_And_Print_Status_From_NM(sock);
                    continue;
                }
                else if (strncmp(request, "read", 4) == 0 || strncmp(request, "write", 5) == 0 || strncmp(request, "get_size_per", 12) == 0)
                {
                    Process_Read_Write_Get_Request(sock, request);
                }

                else if (strncmp(request, "exit", 4) == 0)
                {
                    break;
                }
                else
                {
                    printf("Unknown command\n");
                    break;
                }
            }
        }
    }
    close(sock);
}