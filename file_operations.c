#include "storage_server.h"

char *read_file(char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        perror("Error opening the file");
        return NULL;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    size_t file_size_t = (size_t)file_size;

    // Allocate a buffer to store the file contents
    char *buffer = (char *)malloc(file_size);

    // Read the file into the buffer
    size_t bytes_read = fread(buffer, 1, file_size, file);

    if (bytes_read != file_size_t)
    {
        perror("Error reading the file");
        fclose(file);
        free(buffer);
        return NULL;
    }
    fclose(file);

    return buffer;
}

int write_to_file(char *path, char *contents)
{
    FILE *file = file = fopen(path, "a");

    if (file == NULL)
    {
        perror("Error opening the file");
        return 1;
    }
    size_t data_length = strlen(contents);

    size_t bytes_written = fwrite(contents, 1, data_length, file);

    if (bytes_written != data_length)
    {
        perror("Error writing to the file");
        fclose(file);
        return -1;
    }

    fclose(file);

    return 1;
}

struct size_per *get_file_size_per(char *path)
{
    struct size_per *info = (struct size_per *)malloc(sizeof(struct size_per));

    struct stat file_stat;

    if (stat(path, &file_stat) == 0)
    {
        // Successfully obtained file information

        // File size in bytes
        info->file_size = file_stat.st_size;

        // File permissions
        info->file_permissions = file_stat.st_mode;

        return info;
        // You can check individual permission bits like this:
        // if (file_permissions & S_IRUSR)
        // {
        //     printf("User has read permission.\n");
        // }

        // if (file_permissions & S_IWUSR)
        // {
        //     printf("User has write permission.\n");
        // }

        // if (file_permissions & S_IXUSR)
        // {
        //     printf("User has execute permission.\n");
        // }
    }
    else
    {
        perror("Error obtaining file information");
        return NULL;
    }
}

int createEmptyFile(char *filename)
{
    // Attempt to create an empty file
    int fileDescriptor = creat(filename, S_IRUSR | S_IWUSR);

    if (fileDescriptor == -1)
    {
        perror("Error creating the file");
        return -1;
    }

    close(fileDescriptor);
    return 0;
}

int createEmptyDirectory(char *filename)
{
    if (mkdir(filename, S_IRWXU) == -1)
    {
        perror("Error creating the directory");
        return -1;
    }
    return 0;
}

int deleteFile(char *filename)
{
    if (remove(filename) == 0)
    {
        printf("File '%s' deleted successfully.\n", filename);
        return 0;
    }
    else
    {
        perror("Error deleting the file");
        return -1;
    }
}

int deleteDirectory(char *dirname)
{
    pid_t pid;
    int status;

    if ((pid = fork()) < 0)
    {
        perror("Error forking process");
        return -1;
    }
    else if (pid == 0)
    { // Child process
        // Construct the command for deleting the directory and its contents.
        char *rmArgs[] = {"rm", "-r", dirname, NULL};

        // Execute the "rm -r" command.
        if (execvp("rm", rmArgs) < 0)
        {
            perror("Error executing 'rm' command");
            exit(EXIT_FAILURE);
        }
    }
    else
    { // Parent process
        // Wait for the child process to complete.
        if (waitpid(pid, &status, 0) < 0)
        {
            perror("Error waiting for child process");
            return -1;
        }

        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 0)
            {
                printf("Directory '%s' deleted successfully.\n", dirname);
                return 0;
            }
            else
            {
                perror("Error deleting the directory");
                return -1;
            }
        }
    }

    return -1;
}
