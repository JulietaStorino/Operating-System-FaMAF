#include <stdlib.h>    /* calloc()...                        */
#include <string.h>    /* strlen(), strncat, strcopy()...    */
#include <assert.h>    /* assert()...                        */
#include "strextra.h"  /* Interfaz                           */

char *concatstr(char *s1, char *s2){
    
    assert(s2 != NULL);

    if (s1 != NULL){
        size_t len_s1=strlen(s1);
        size_t len_s2=strlen(s2);
        s1 = (char *)realloc(s1, (len_s1 + len_s2 + 1) * sizeof(char)); 
        if (s1 == NULL){
            exit(EXIT_FAILURE);
        }
        strcat(s1, s2);
    }
    
    return s1;
}
//void *reallocarray(void *ptr, size_t nelem, size_t elsize);

char * strmerge(char *s1, char *s2) {
    char *merge=NULL;
    size_t len_s1=strlen(s1);
    size_t len_s2=strlen(s2);
    assert(s1 != NULL && s2 != NULL);
    merge = calloc(len_s1 + len_s2 + 1, sizeof(char));
    strncpy(merge, s1, len_s1);
    merge = strncat(merge, s2, len_s2);
    assert(merge != NULL && strlen(merge) == strlen(s1) + strlen(s2));
    return merge;
}
