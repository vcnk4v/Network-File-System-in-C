#include "ns.h"

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

void Copy_File_redundancy(char *buffer, SS original, SS backup)
{
    // Identify source and destination
    char todo[32];
    char filename[4096];
    char dest_path[4096];
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
    SS ss_for_file = original;
    SS ss_for_dest = backup;

    int storage_server_socket_src = ss_for_file->socket;
    int storage_server_socket_dest = ss_for_dest->socket;

    char make_create_instr[4096] = "create_file ";
    char newfile_path[4096];
    bzero(newfile_path, sizeof(newfile_path));

    strcpy(newfile_path, dest_path);

    char *file = extract_filename(filename);
    strcat(newfile_path, file);

    strcat(make_create_instr, newfile_path);
    // Ask_SS_To_Create_File(make_create_instr, client, 0);

    send(storage_server_socket_dest, make_create_instr, sizeof(make_create_instr), 0);
    char status[256];
    bzero(status, sizeof(status));
    recv(storage_server_socket_dest, status, sizeof(status), 0);

    char make_read_instr[4096] = "read ";
    strcat(make_read_instr, filename);

    send(storage_server_socket_src, make_read_instr, sizeof(make_read_instr), 0);

    Handle_Read_Request(storage_server_socket_src, storage_server_socket_dest, newfile_path);

    // Create new file in destination_SS

    // Write to this file in destination_SS
    // char make_write_instr[4096] = "write ";
    // strcat(make_write_instr, newfile_path);

    // strcat(make_write_instr, " ");

    // strcat(make_write_instr, file_contents);

    // send(storage_server_socket_dest, make_write_instr, sizeof(make_write_instr), 0);

    // // char status2[256];  // from
    // recv(storage_server_socket_dest, status, sizeof(status), 0); // from handle_write_request in SS

    // // Update the accessible paths of destination_SS

    // Send status of priviledged operation to client
}

char *create_folders_redundancy(char *buffer, char *dir_made_till_now, SS backup)
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
    // Ask_SS_To_Create_Folder(make_create_command, client, 0);
    send(backup->socket, make_create_command, sizeof(make_create_command), 0);
    char status[256];
    recv(backup->socket, status, sizeof(status), 0);

    strcat(dir_made_till_now, "/");
    return dir_made_till_now;
}

void Copy_Folder_redundancy(char *src_dir, SS original, SS backup)
{
    // char todo[16];
    // char src_dir[4096];
    // char dest_dir[4096];

    // Identify source and destination directory
    // sscanf(buffer, "%s %s %s", todo, src_dir, dest_dir);

    if (src_dir[strlen(src_dir) - 1] != '/')
    {
        src_dir[strlen(src_dir)] = '/';
        src_dir[strlen(src_dir) + 1] = '\0';
    }

    int n;
    char **s = subpaths_in_trie(&n, src_dir);

    char *dir_made_till_now = (char *)malloc(sizeof(char) * 4096);
    bzero(dir_made_till_now, sizeof(dir_made_till_now));
    char dir_made_till_now_mandatory[4096];
    bzero(dir_made_till_now_mandatory, sizeof(dir_made_till_now_mandatory));
    strcpy(dir_made_till_now, "");

    char src_dir_copy[4096];
    bzero(src_dir_copy, sizeof(src_dir_copy));
    strcpy(src_dir_copy, src_dir);

    strcpy(dir_made_till_now_mandatory, create_folders_redundancy(src_dir_copy, dir_made_till_now, backup));
    strcpy(dir_made_till_now, dir_made_till_now_mandatory);

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
            Copy_File_redundancy(make_command, original, backup);
        }
        else
        {
            // handle dird1/dirc1
            char *x = create_folders_inner_red(without_src_dir[i], dir_made_till_now);

            if (strcmp(x, "") == 0)
            {
                char make_command[4096] = "create_folder ";
                strcat(make_command, dir_made_till_now_mandatory);
                strcat(make_command, without_src_dir[i]);

                strcpy(dir_made_till_now, dir_made_till_now_mandatory);
                strcat(dir_made_till_now, without_src_dir[i]);

                if (make_command[strlen(make_command) - 1] == '/')
                    make_command[strlen(make_command) - 1] = '\0';

                // Ask_SS_To_Create_Folder(make_command, client, 0);

                send(backup->socket, make_command, sizeof(make_command), 0);
                char status[256];
                recv(backup->socket, status, sizeof(status), 0);
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

                    // Ask_SS_To_Create_Folder(make_command, client, 0);

                    send(backup->socket, make_command, sizeof(make_command), 0);
                    char status[256];
                    recv(backup->socket, status, sizeof(status), 0);
                }
                else
                {
                    char make_command[4096] = "copy_file ";
                    strcat(make_command, s[i]);

                    char *till_last_slash = (char *)malloc(sizeof(char) * 4096);
                    extractSubstring(s[i], till_last_slash);

                    char *found = strstr(dir_made_till_now, till_last_slash);
                    int start = found - dir_made_till_now;
                    int end = start + strlen(till_last_slash) - 1;

                    for (unsigned int j = end + 1; j < strlen(dir_made_till_now); j++)
                    {
                        dir_made_till_now[j] = '\0';
                    }

                    strcat(make_command, " ");
                    strcat(make_command, dir_made_till_now);

                    if (make_command[strlen(make_command) - 1] == '/')
                    {
                        make_command[strlen(make_command) - 1] = '\0';
                    }
                    else
                    {
                        make_command[strlen(make_command)] = '\0';
                    }
                    Copy_File_redundancy(make_command, original, backup);
                }
            }
        }
    }
}

void get_all_path_depth1(Trie root, char *prefix, int *num_paths, char **subpaths, int ss_index) // modify so that you can do till depth
{
    if (!root)
        return;

    if (root->SS_index == ss_index)
    {
        strcpy(subpaths[*num_paths], prefix);
        (*num_paths)++;
        return;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (root->children[i])
        {
            char next_prefix[4096];
            bzero(next_prefix, sizeof(next_prefix));
            strcpy(next_prefix, prefix);
            char temp[2] = {get_char_from_index(i), '\0'};
            strcat(next_prefix, temp);
            get_all_path_depth1(root->children[i], next_prefix, num_paths, subpaths, ss_index);
        }
    }
}

void duplicate_accessible_paths(SS ss)
{
    Trie current = accessible_paths_trie;
    int max_no_of_paths = 128;
    char **all_subpaths = (char **)malloc(sizeof(char *) * max_no_of_paths);
    for (int i = 0; i < max_no_of_paths; i++)
    {
        all_subpaths[i] = (char *)malloc(sizeof(char) * 4096);
        bzero(all_subpaths[i], sizeof(all_subpaths[i]));
    }

    int num_paths = 0; // Reset the number of paths
    get_all_path_depth1(current, "", &num_paths, all_subpaths, ss->SS_index);

    for (int i = 0; i < num_paths; i++)
    {
        if (ss->backups[0]->is_active)
        {
            printf("1 backup made\n");
            Copy_Folder_redundancy(all_subpaths[i], ss, ss->backups[0]);
        }
        if (ss->backups[1]->is_active)
        {
            printf("2 backup made\n");
            Copy_Folder_redundancy(all_subpaths[i], ss, ss->backups[1]);
        }
    }
}

void get_all_path_depth(Trie root, char *prefix, int *num_paths, char **subpaths, int ss_index) // modify so that you can do till depth
{
    if (!root)
        return;

    if (root->SS_index == ss_index)
    {
        strcpy(subpaths[*num_paths], prefix);
        (*num_paths)++;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (root->children[i])
        {
            char next_prefix[4096];
            bzero(next_prefix, sizeof(next_prefix));
            strcpy(next_prefix, prefix);
            char temp[2] = {get_char_from_index(i), '\0'};
            strcat(next_prefix, temp);
            get_all_path_depth(root->children[i], next_prefix, num_paths, subpaths, ss_index);
        }
    }
}