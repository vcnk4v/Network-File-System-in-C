#include "../../../storage_server.h"
#include "../../../defs.h"
#define CLIENT_PORT 18202 // Port for client connection

#define MYPATHS 3

char accessiblePaths[MAX_PATHS][4096];

struct file_access **file_semaphores;

int file_semaphores_index;

int client_sockets[MAX_CLIENTS];
pthread_t client_threads[MAX_CLIENTS];
int client_count = 0;

sem_t count_lock;

void initializeAccessiblePaths()
{
    // Initialize the list of accessible paths on SS_1
    strcpy(accessiblePaths[0], "dir_ctpcj/dir_apsck/file_hwa.txt");
    strcpy(accessiblePaths[1], "dir_ctpcj/dir_apsck/file_rkn.txt");
    strcpy(accessiblePaths[2], "dir_ctpcj/dir_hfccx/here.txt");
    // Add more paths as needed
}

void initialise_file_semaphores(char *all_paths)
{
    file_semaphores = (struct file_access **)malloc(sizeof(struct file_access *) * MAX_PATHS);
    for (int i = 0; i < MAX_PATHS; i++)
    {
        file_semaphores[i] = (struct file_access *)malloc(sizeof(struct file_access));
    }
    // printf("%s..\n",all_paths);

    char *token = strtok(all_paths, "\n");
    while (token != NULL)
    {
        if (token[strlen(token) - 1] != '/')
        {
            strcpy(file_semaphores[file_semaphores_index]->filepath, token);
            sem_init(&file_semaphores[file_semaphores_index]->writelock, 0, 1);
            sem_init(&file_semaphores[file_semaphores_index]->readlock, 0, 1);
            file_semaphores[file_semaphores_index]->readercount = 0;
            file_semaphores_index++;
        }
        token = strtok(NULL, "\n");
    }

    // for (int i = 0; i < MYPATHS; i++)
    // {
    //     strcpy(file_semaphores[i]->filepath, accessiblePaths[i]);
    //     sem_init(&file_semaphores[i]->writelock, 0, 1);
    //     sem_init(&file_semaphores[i]->readlock, 0, 1);
    //     file_semaphores[i]->readercount = 0;
    // }
}

int Check_File_Lock_State(char *path)
{
    struct file_access *cur_file;
    for (int i = 0; i < file_semaphores_index; i++)
    {
        cur_file = file_semaphores[i];
        if (strcmp(cur_file->filepath, path) == 0)
        {
            return i;
        }
    }
    return -1;
}

void register_server(int nmSocket)
{
    initializeAccessiblePaths();

    file_semaphores_index = 0;

    char ssDetails[MAX_REGISTER_SIZE];
    char paths[MAX_SUM_OF_ALL_PATHS] = "";
    for (int i = 0; i < MYPATHS; i++)
    {
        strcat(paths, accessiblePaths[i]);
        if (i != MYPATHS - 1)
        {
            strcat(paths, " ");
            if (i != MYPATHS - 1)
            {
                strcat(paths, " ");
            }
        }
    }
    printf("%s\n", paths);
    sprintf(ssDetails, "REGISTER\n%s\n%d\n%d\n%s", NM_IP, NM_PORT, CLIENT_PORT, paths);
    send(nmSocket, ssDetails, strlen(ssDetails), 0);
    printf("SS details sent to the Naming Server.\n");
    char message[128];
    bzero(message, sizeof(message));
    // if (recv(nmSocket, message, sizeof(message), 0) < 0)
    // {
    //     perror("Error in receiving confirmation for registration");
    //     exit(EXIT_FAILURE);
    // }
    // if (strcmp(message, "registered") == 0)
    // {
    //     printf("Registration complete\n");
    // }

    char all_paths[MAX_SUM_OF_ALL_PATHS];
    bzero(all_paths, sizeof(all_paths));

    if (recv(nmSocket, all_paths, sizeof(all_paths), 0) < 0)
    {
        perror("Error in receiving confirmation for registration");
        exit(EXIT_FAILURE);
    }
    printf("All paths recieved\n");
    // printf("%s\n",all_paths);

    initialise_file_semaphores(all_paths);
}

int Handle_Read_Request(char *buffer, int client_connection)
{
    char path[4096];

    char *token = strtok(buffer, " ");
    strcpy(path, strtok(NULL, " "));

    (void)token;
    int check_index = Check_File_Lock_State(path);
    struct file_access *cur_file = file_semaphores[check_index];

    sem_wait(&cur_file->readlock);

    cur_file->readercount++;

    if (cur_file->readercount == 1)
        sem_wait(&cur_file->writelock);

    sem_post(&cur_file->readlock);
    FILE *file = fopen(path, "r");
    size_t file_size;
    if (file == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    // Calculate the file size
    fseek(file, 0, SEEK_END);                // Move the file pointer to the end
    file_size = ftell(file);                 // Get the current position, which is the file size
    rewind(file);                            // Reset file pointer to the beginning
    buffer = (char *)malloc(CHUNK_SIZE + 1); // +1 for null terminator

    // size_t elements_read = fread(buffer, sizeof(char), file_size, file);
    // if (elements_read != file_size)
    // {
    //     perror("Error reading the file");
    //     free(buffer);
    //     fclose(file);
    //     return EXIT_FAILURE;
    // }

    // buffer[file_size] = '\0';

    // char file_contents[CHUNK_SIZE];
    long unsigned buffer_index = 0;

    send(client_connection, &file_size, sizeof(long), 0);

    // Loop to read and send file contents in chunks
    while (buffer_index < file_size)
    {
        if (buffer_index + CHUNK_SIZE <= file_size)
        {
            bzero(buffer, sizeof(buffer));
            size_t elements_read = fread(buffer, sizeof(char), CHUNK_SIZE, file);
            buffer[CHUNK_SIZE] = '\0';
            // strncpy(file_contents, buffer + buffer_index, CHUNK_SIZE);
            buffer_index += CHUNK_SIZE;

            if (send(client_connection, buffer, CHUNK_SIZE, 0) < 0)
            {
                perror("Error in sending contents of file from storage server to client");
                fclose(file);
                return -1;
            }
        }

        else
        {
            // strcpy(file_contents, buffer + buffer_index);

            bzero(buffer, sizeof(buffer));
            size_t elements_read = fread(buffer, sizeof(char), file_size - buffer_index, file);

            buffer[CHUNK_SIZE] = '\0';
            buffer_index = file_size;
            int sent = send(client_connection, buffer, CHUNK_SIZE, 0);
            if (sent < 0)
            {
                perror("Error in sending contents of file from storage server to client");
                fclose(file);
                return -1;
            }
            // printf("%d sent\n", sent);
        }
    }
    sem_wait(&cur_file->readlock);
    cur_file->readercount--;

    if (cur_file->readercount == 0)
    {
        sem_post(&cur_file->writelock);
    }

    // Lock the semaphore
    sem_post(&cur_file->readlock);
    return 1;
}

int Handle_Write_Request(char *buffer, int client_connection)
{
    char path[4096];

    char *token = strtok(buffer, " ");
    strcpy(path, strtok(NULL, " "));

    (void)token;

    // Check if you can write to file
    int check_index = Check_File_Lock_State(path);
    struct file_access *cur_file = file_semaphores[check_index];

    int wait = sem_trywait(&cur_file->writelock);
    if (wait == -1)
    {
        printf("In trywait\n");
        if (errno == EAGAIN)
        {
            printf("In EAGAIN\n");
            char busy_message[4096] = "Another client is writing to the file. Please try again after some time.";
            send(client_connection, busy_message, sizeof(busy_message), 0);
            return -1;
        }
        else
        {
            perror("sem_trywait");
        }
    }
    else
    {

        // sem_wait(&cur_file->writelock);

        int value;
        sem_getvalue(&cur_file->writelock, &value);
        printf("%d\n", value);

        char message[6] = "Ready";
        send(client_connection, message, sizeof(message), 0);

        char contents[CHUNK_SIZE + 1] = "";

        while (strncmp(contents, "STOP", 4) != 0)
        {
            write_to_file(path, contents);
            recv(client_connection, contents, sizeof(contents), 0);
        }

        sem_post(&cur_file->writelock);

        char msg[16] = "Success";
        if (send(client_connection, msg, sizeof(msg), 0) < 0)
        {
            perror("Error in sending contents of file from storage server to client");
            return -1;
        }
    }
    return 1;
}

void Handle_Write_Request_From_NM(char *buffer, int nm)
{
    char write[6];
    char filepath[4096];
    char contents[4096];

    char *token = strtok(buffer, " ");
    strcpy(write, token);
    strcpy(filepath, strtok(NULL, " "));
    strcpy(contents, strtok(NULL, "\0"));
    if (contents == NULL || filepath == NULL)
    {
        char status[16];
        strcpy(status, "Success");

        send(nm, status, sizeof(status), 0);
        return;
    }

    write_to_file(filepath, contents);

    char status[16];
    strcpy(status, "Success");

    send(nm, status, sizeof(status), 0);
    return;
}

int Handle_Size_Per_Request(char *buffer, int client_connection)
{
    char path[4096];
    char *token = strtok(buffer, " ");
    strcpy(path, strtok(NULL, " "));

    (void)token;

    struct size_per *info = get_file_size_per(path);
    // printf("%o..\n",info->file_permissions);
    // printf("%ld..\n",info->file_size);

    if (send(client_connection, info, sizeof(struct size_per), 0) < 0)
    {
        perror("Error in sending file size and permissions to client from storage server");
        return -1;
    }

    char ack[5] = "STOP";
    send(client_connection, ack, sizeof(ack), 0);
    return 1;
}

void *handle_client_request(void *arg)
{
    int client_connection = *(int *)arg;

    char buffer[4096];
    bzero(buffer, sizeof(buffer));
    int bytes_rec = recv(client_connection, buffer, sizeof(buffer), 0);

    if (bytes_rec < 0)
    {
        perror("Error in recieving request from client");
        sem_wait(&count_lock);
        client_count--;
        sem_post(&count_lock);
        close(client_connection);
    }
    else if (bytes_rec == 0)
    {
        // printf("Bye\n");
        sem_wait(&count_lock);
        client_count--;
        sem_post(&count_lock);
        close(client_connection);

        return NULL;
    }

    if (strncmp(buffer, "read", 4) == 0)
    {
        Handle_Read_Request(buffer, client_connection);
    }
    else if (strncmp(buffer, "write", 5) == 0)
    {
        Handle_Write_Request(buffer, client_connection);
    }
    else if (strncmp(buffer, "get_size_per", 12) == 0)
    {
        Handle_Size_Per_Request(buffer, client_connection);
    }
    else if (strcmp(buffer, "nmmssg") == 0)
    {
    }

    else
    {
        printf("Unknown command\n");
    }
    close(client_connection);

    sem_wait(&count_lock);
    client_count--;
    sem_post(&count_lock);

    return NULL;
}

void *handle_clients()
{
    int client_socket, client_connection;
    struct sockaddr_in client_addr, client_diff_addr;
    socklen_t client_diff_addr_Len;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Client socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&client_addr, 0, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
    {
        perror("Client socket bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(client_socket, 10) == -1)
    {
        perror("Client socket listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Storage Server listening for clients on port %d...\n", CLIENT_PORT);

    while (1)
    {
        if (client_count >= MAX_CLIENTS)
        {
            printf("Maximum clients reached. No longer accepting connections.\n");
            continue;
        }

        client_diff_addr_Len = sizeof(client_diff_addr);
        client_connection = accept(client_socket, (struct sockaddr *)&client_diff_addr, &client_diff_addr_Len);

        if (client_connection == -1)
        {
            perror("Accept failed");
            continue;
        }

        sem_wait(&count_lock);
        int index_client = client_count++;

        if (pthread_create(&client_threads[index_client], NULL, handle_client_request, (void *)&client_connection) != 0)
        {
            perror("pthread_create");
            client_count--;
            sem_post(&count_lock);
            break;
        }

        sem_post(&count_lock);

        // handle_client_request(client_connection);
        // close(client_connection);
    }
    sem_wait(&count_lock);
    int index_cl = client_count;
    sem_post(&count_lock);

    for (int i = 0; i < index_cl; i++)
    {
        pthread_join(client_threads[i], NULL);
    }
    close(client_socket);
    return NULL;
}

void Handle_Create_File_Request(char *buffer, int nm_socket)
{
    char path[4096];

    char *token = strtok(buffer, " ");
    strcpy(path, strtok(NULL, " "));

    (void)token;

    char status[16];

    if (createEmptyFile(path) < 0)
    {
        strcpy(status, "Failure");
    }
    else
    {
        strcpy(status, "Success");

        strcpy(file_semaphores[file_semaphores_index]->filepath, path);
        sem_init(&file_semaphores[file_semaphores_index]->writelock, 0, 1);
        sem_init(&file_semaphores[file_semaphores_index]->readlock, 0, 1);
        file_semaphores[file_semaphores_index]->readercount = 0;
        file_semaphores_index++;
    }
    send(nm_socket, status, sizeof(status), 0);
}

void Handle_Create_Folder_Request(char *buffer, int nm_socket)
{
    char path[4096];

    char *token = strtok(buffer, " ");
    strcpy(path, strtok(NULL, " "));

    (void)token;

    char status[16];
    if (createEmptyDirectory(path) < 0)
    {
        strcpy(status, "Failure");
    }
    else
    {
        strcpy(status, "Success");
    }
    send(nm_socket, status, sizeof(status), 0);
}

void Handle_Delete_File_Request(char *buffer, int nm_socket)
{
    char path[4096];

    char *token = strtok(buffer, " ");
    strcpy(path, strtok(NULL, " "));

    (void)token;

    char status[16];
    if (deleteFile(path) < 0)
    {
        strcpy(status, "Failure");
    }
    else
    {
        strcpy(status, "Success");
    }
    send(nm_socket, status, sizeof(status), 0);
}

void Handle_Delete_Folder_Request(char *buffer, int nm_socket)
{
    char path[4096];

    char *token = strtok(buffer, " ");
    strcpy(path, strtok(NULL, " "));

    (void)token;

    char status[16];
    if (deleteDirectory(path) < 0)
    {
        strcpy(status, "Failure");
    }
    else
    {
        strcpy(status, "Success");
    }
    send(nm_socket, status, sizeof(status), 0);
}

int Create_Socket_To_NM()
{
    int nm_socket;
    struct sockaddr_in nm_addr;

    if ((nm_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Naming Server socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&nm_addr, 0, sizeof(nm_addr));

    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(NM_PORT);
    nm_addr.sin_addr.s_addr = inet_addr(NM_IP);

    if (connect(nm_socket, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) < 0)
    {
        perror("Connection with the Naming Server failed");
        exit(1);
    }
    return nm_socket;
}

void *handle_naming_server()
{
    int nm_socket = Create_Socket_To_NM();

    register_server(nm_socket);

    while (1)
    {
        char buffer[4096];
        bzero(buffer, sizeof(buffer));
        int bytes_rec = recv(nm_socket, buffer, sizeof(buffer), 0);
        if (bytes_rec < 0)
        {
            perror("Error in receiving");
            exit(EXIT_FAILURE);
        }

        if (strncmp(buffer, "create_file", 11) == 0)
        {
            Handle_Create_File_Request(buffer, nm_socket);
        }
        else if (strncmp(buffer, "create_folder", 13) == 0)
        {
            Handle_Create_Folder_Request(buffer, nm_socket);
        }
        else if (strncmp(buffer, "delete_file", 11) == 0)
        {
            Handle_Delete_File_Request(buffer, nm_socket);
        }
        else if (strncmp(buffer, "delete_folder", 13) == 0)
        {
            Handle_Delete_Folder_Request(buffer, nm_socket);
        }
        else if (strncmp(buffer, "read", 4) == 0)
        {
            Handle_Read_Request(buffer, nm_socket);
        }
        else if (strncmp(buffer, "write", 5) == 0)
        {
            Handle_Write_Request_From_NM(buffer, nm_socket);
        }
    }
    close(nm_socket);
}

int main()
{
    sem_init(&count_lock, 0, 1);
    pthread_t naming_server_thread, client_thread;
    pthread_create(&naming_server_thread, NULL, handle_naming_server, NULL);
    pthread_create(&client_thread, NULL, handle_clients, NULL);

    pthread_join(naming_server_thread, NULL);
    pthread_join(client_thread, NULL);
}
