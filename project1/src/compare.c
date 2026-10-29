#include <stdio.h>
#include <string.h>

#include "compare.h"

int find_missing(const FileList *src, const FileList *dst, FileList *out)
{
    size_t i = 0;  /* position in src */
    size_t j = 0;  /* position in dst */

    while (i < src->count) {
        /* dst is used up, so everything left in src is missing. */
        if (j >= dst->count) {
            if (filelist_add(out, src->names[i]) != 0) {
                fprintf(stderr, "compare: out of memory\n");
                return -1;
            }
            i++;
            continue;
        }

        int order = strcmp(src->names[i], dst->names[j]);

        if (order == 0) {
            /* Same name in both: nothing to sync, advance both. */
            i++;
            j++;
        } else if (order < 0) {
            /* src name sorts before dst name, so dst does not have it. */
            if (filelist_add(out, src->names[i]) != 0) {
                fprintf(stderr, "compare: out of memory\n");
                return -1;
            }
            i++;
        } else {
            /* dst name sorts before src name: it is extra, skip it. */
            j++;
        }
    }

    return 0;
}