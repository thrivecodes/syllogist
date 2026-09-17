#include "util.h"
#include <stdlib.h>
#include <string.h>

char* syllogist_strdup(const char *s) {
    if (!s) {
        char *empty = (char *)malloc(1);
        if (empty) {
            empty[0] = '\0';
        }
        return empty;
    }
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1);
    if (copy) {
        memcpy(copy, s, len + 1);
    }
    return copy;
}
