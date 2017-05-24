#ifndef DEBUG_H
#define DEBUG_H
#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG
#define debug(S, ...)   do{fprintf(stderr, KMAG "DEBUG: %s:%s:%d " KNRM S,   __FILE__, __extension__ __FUNCTION__, __LINE__, __VA_ARGS__  );}while(0)
#define error(S, ...)   do{fprintf(stderr, KRED "ERROR: %s:%s:%d " KNRM S,   __FILE__, __extension__ __FUNCTION__, __LINE__, __VA_ARGS__  );}while(0)
#define info(S, ...)    do{fprintf(stderr, KBLU "INFO: %s:%s:%d " KNRM S,    __FILE__, __extension__ __FUNCTION__, __LINE__, __VA_ARGS__  );}while(0)
#else
#define debug(S, ...)
#define error(S, ...)
#define info(S, ...)
#endif

#endif /* DEBUG_H */
