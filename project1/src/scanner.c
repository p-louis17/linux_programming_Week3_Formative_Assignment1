#define _DEFAULT_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"

#define INITIAL_CAPACITY 16

void filelist_init(FileList *list)
{
    list->names = NULL;
    list->count = 0;
    list->capacity = 0;
}

void filelist_free(FileList *list)
{
    for (size_t i = 0; i < list->count; i++) {
        free(list->names[i]);
    }
    free(list->names);
    filelist_init(list);
}

/* Append a copy of name. Doubles the array when full. */
int filelist_add(FileList *list, const char *name)
{
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : INITIAL_CAPACITY;
        char **grown = realloc(list->names, new_cap * sizeof(char *));
        if (grown == NULL) {
            return -1;
        }
        list->names = grown;
        list->capacity = new_cap;
    }

    list->names[list->count] = strdup(name);
    if (list->names[list->count] == NULL) {
        return -1;
    }
    list->count++;
    return 0;
}

/* qsort comparator: orders two string pointers with strcmp. */
static int compare_names(const void *a, const void *b)
{
    const char *const *sa = a;
    const char *const *sb = b;
    return strcmp(*sa, *sb);
}

int scan_directory(const char *dir_path, FileList *out)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip the current and parent directory entries. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (filelist_add(out, entry->d_name) != 0) {
            fprintf(stderr, "scanner: out of memory\n");
            closedir(dir);
            return -1;
        }
    }

    closedir(dir);

    /* Sorted lists let the compare module walk both in one pass. */
    qsort(out->names, out->count, sizeof(char *), compare_names);
    return 0;
}