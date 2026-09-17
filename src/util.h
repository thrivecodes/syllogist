#ifndef SYLLOGIST_UTIL_H
#define SYLLOGIST_UTIL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Portable, safe string duplication.
 * If s is NULL, allocates and returns an empty string ("").
 * Returns NULL on memory allocation failure.
 */
char* syllogist_strdup(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* SYLLOGIST_UTIL_H */
