#include "ns.h"

// #define ALPHABET_SIZE 68
// struct trie_node
// {
//     int SS_index[3];                           // store primary SS and 2 backups
//     struct trie_node *children[ALPHABET_SIZE]; // 52 alphabets + 10 numbers + '/' + '.'
// };

// typedef struct trie_node *Trie;
// Trie make_node(int SS_index1);
// int insert(Trie t, char *str, int len, int *SS_index);
// int *search_trie(Trie root, char *str, int len); // returns indices of SS and the two backup SS
// int search_helper(Trie root, char *path, int operation_flag);
// int delete_f(Trie root, char *fname, int opn_flag);

// Trie accessible_paths_trie;
// SS storage_server;
/*

ASCII:

45 - 57: -,.,/,0,1,2...9 : 0-12
63 - 90: ?,@,A,B,...Z : 13-40
95: _ : 41
97-122 : a,...z : 42 - 67

also ensure in insertion --> if a folder is being inserted/deleted, its path has to end with '/'
*/

int get_char_index(char c)
{
    int ind;
    if (c > 44 && c < 58)
    {
        ind = c - '-';
    }
    else if (c > 62 && c < 91)
    {
        ind = c - '-' - 5;
    }
    else if (c == 95)
    {
        ind = 41;
    }
    else if (c > 96 && c < 123)
    {
        ind = c - '-' - 10;
    }
    else
    {
        return -1; // invalid character
    }
    return ind;
}

Trie make_node(int SS_index1)
{
    Trie n = (Trie)malloc(sizeof(struct trie_node));
    n->SS_index = SS_index1;
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        n->children[i] = NULL;
    }
    return n;
}

int insert(char *str, int len, int SS_index)
{
    Trie pointer = accessible_paths_trie;
    for (int i = 0; i < len; i++)
    {
        int ind = get_char_index(str[i]);
        if (ind == -1)
        {
            return -1;
        }

        Trie next = pointer->children[ind];
        if (next == NULL)
        {
            pointer->children[ind] = make_node(-1);
            next = pointer->children[ind];
        }
        pointer = next;
        if (str[i] == '/')
        {
            pointer->SS_index = SS_index;
        }
    }
    pointer->SS_index = SS_index;

    return 1;
}

int is_empty(Trie root)
{
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (root->children[i] != NULL)
        {
            return 0;
        }
    }
    return 1;
}

void delete_all(Trie node)
{
    if (node == NULL)
    {
        return;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (node->children[i])
        {
            delete_all(node->children[i]);
        }
    }
    free(node);
}

Trie remove_path(Trie root, char *word, int depth)
{
    if (root == NULL)
    {
        return NULL;
    }

    if (depth == (int)strlen(word))
    {
        if (root->SS_index != -1)
        {
            root->SS_index = -1;

            if (is_empty(root))
            {
                free(root);
                root = NULL;
            }
        }
        return root;
    }

    int ind = get_char_index(word[depth]);
    root->children[ind] = remove_path(root->children[ind], word, depth + 1);

    if (is_empty(root) && root->SS_index != -1)
    {
        free(root);
        root = NULL;
    }

    return root;
}

int delete_f(Trie root, char *fname, int opn_flag)
{
    Trie current = root;
    if (opn_flag == 0) // file
    {
        remove_path(current, fname, 0);
    }
    else // folder
    {
        int ind;
        for (int i = 0; i < (int)strlen(fname); i++)
        {
            ind = get_char_index(fname[i]);
            if (ind == -1)
            {
                return -1;
            }
            Trie next = current->children[ind];
            if (!next)
            {
                return -1;
            }
            current = next;
        }
        for (int i = 0; i < ALPHABET_SIZE; i++)
        {
            delete_all(current->children[i]);
        }
    }
    return 1;
}

int search_trie(char *str, int len) // returns indices of SS and the two backup SS
{
    int ind;
    Trie current = accessible_paths_trie;
    for (int i = 0; i < len; i++)
    {
        ind = get_char_index(str[i]);

        if (ind == -1)
        {
            return -1;
        }

        Trie next = current->children[ind];
        if (next == NULL)
        {
            return -1;
        }
        current = next;
    }
    if (current == NULL)
    {
        return -1;
    }

    return current->SS_index;
}

SS search_helper_trie(char *path, int operation_flag)
{
    int ss_index;
    if (operation_flag == 0) // delete file/folder or read or write or get_size_per
    {
        ss_index = search_trie(path, strlen(path));
    }
    else if (operation_flag == 1) // create file
    {
        char *lastSlash = strrchr(path, '/');
        int lengthUpToLastSlash = lastSlash - path + 1;
        ss_index = search_trie(path, lengthUpToLastSlash);
    }

    else if (operation_flag == 2) // create folder
    {
        char temp[4096];
        strncpy(temp, path, strlen(path) - 1);
        char *lastSlash = strrchr(temp, '/');
        int lengthUpToLastSlash = lastSlash - temp + 1;
        ss_index = search_trie(path, lengthUpToLastSlash);
    }

    if (ss_index == -1)
    {
        return NULL;
    }

    SS ss_for_file = NULL;
    SS ss_ptr = storage_server->next;
    while (ss_ptr != NULL)
    {
        if (ss_ptr->SS_index == ss_index)
        {
            ss_for_file = ss_ptr;
            // ss_ptr;
            break;
        }
        ss_ptr = ss_ptr->next;
    }

    return ss_for_file;
}

SS search_backups(SS unavailable_ss)
{
    if (unavailable_ss->backups[0] != NULL && unavailable_ss->backups[0]->is_active != 0)
    {
        return unavailable_ss->backups[0];
    }
    else if (unavailable_ss->backups[1] != NULL && unavailable_ss->backups[1]->is_active != 0)
    {
        return unavailable_ss->backups[1];
    }
    return NULL;
}

int create_folder(int ss_index, char *fname)
{

    if (fname[strlen(fname) - 1] != '/')
    {
        // fname[strlen(fname)];
        fname[strlen(fname) + 1] = '\0';
    }
    if (insert(fname, strlen(fname), ss_index) < 0)
    {
        return -1;
    }
    return 0;
}

int create_file(int ss_index, char *path)
{
    if (insert(path, strlen(path), ss_index) < 0)
    {
        return -1;
    }
    return 0;
}

int delete_file(char *path)
{
    char *fname = strrchr(path, '/');
    char file[4096];
    if (fname != NULL)
    {
        strncpy(file, fname + 1, strlen(fname) - 1);
    }
    else
    {
        strcpy(file, path);
    }
    if (delete_f(accessible_paths_trie, file, 0) < 0)
    {
        return -1;
    }
    return 0;
}

int delete_folder(char *fname)
{
    if (fname[strlen(fname) - 1] != '/')
    {
        fname[strlen(fname)] = '/';
        fname[strlen(fname) + 1] = '\0';
    }
    if (delete_f(accessible_paths_trie, fname, 1) < 0)
    {
        return -1;
    }
    return 0;
}

char get_char_from_index(int index)
{
    char c;
    if (index >= 0 && index <= 12)
    {
        c = index + '-';
    }
    else if (index >= 13 && index <= 40)
    {
        c = index + '-' + 5;
    }
    else if (index == 41)
    {
        c = '_';
    }
    else if (index >= 42 && index < 68)
    {
        c = index + '-' + 10;
    }
    else
    {
        c = '\0'; // Invalid index
    }
    return c;
}

void get_all_paths(Trie root, char *prefix, int *num_paths, char **subpaths)
{
    if (!root)
        return;

    if (root->SS_index != -1)
    {
        strcpy(subpaths[*num_paths], prefix);
        (*num_paths)++;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++)
    {
        if (root->children[i])
        {
            char next_prefix[4096];
            strcpy(next_prefix, prefix);
            char temp[2] = {get_char_from_index(i), '\0'};
            strcat(next_prefix, temp);
            get_all_paths(root->children[i], next_prefix, num_paths, subpaths);
        }
    }
}

// paths for copy
char **subpaths_in_trie(int *num_paths, char *parent)
{
    int max_no_of_paths = 128;
    char **all_subpaths = (char **)malloc(sizeof(char *) * max_no_of_paths);
    for (int i = 0; i < max_no_of_paths; i++)
    {
        all_subpaths[i] = (char *)malloc(sizeof(char) * 1024);
        bzero(all_subpaths[i], sizeof(all_subpaths[i]));
    }
    int ind;
    Trie current = accessible_paths_trie;

    for (int i = 0; i < (int)strlen(parent); i++)
    {
        ind = get_char_index(parent[i]);
        if (ind == -1)
        {
            *num_paths = 0;
            return NULL;
        }
        Trie next = current->children[ind];
        if (!next)
        {
            *num_paths = 0;
            return NULL;
        }
        current = next;
    }
    if (is_empty(current))
    {
        *num_paths = 0;
        return NULL;
    }
    *num_paths = 0; // Reset the number of paths
    get_all_paths(current, parent, num_paths, all_subpaths);

    return all_subpaths;
}