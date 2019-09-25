/**
 * Mini Memcheck
 * CS 241 - Fall 2019
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
int main() {
    // Your tests here using malloc and free

    //attempt to allocate and free 4 bytes
    void * basic_malloc = malloc(4);
    free(basic_malloc);

    //attempt to free NULL - invalid_addresses == 1?
    free(NULL);

    //attempt double-free - invalid_addresses == 2?
    free(basic_malloc);

    //attempt to malloc 0 bytes
    void * zero_malloc = malloc(0);
    assert(zero_malloc == NULL);
    
    //calloc 0 elems
    void * zero_calloc = calloc(0, 0);
    assert(zero_calloc == NULL);
    
    //calloc 4 elems
    void * basic_calloc = calloc(4, 4);
    int idx = 0;
    for (; idx < 4; idx++) {
        int * current_elem = (int *) (basic_calloc + (idx * 4));
        assert(*current_elem == 0);
    }
    
    free(basic_calloc);
    
    void * small_malloc = malloc(10);
    small_malloc = realloc(small_malloc, 11);
    free(small_malloc);
    return 0;
}
