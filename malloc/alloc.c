/**
 * Malloc
 * CS 241 - Fall 2019
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


extern size_t MEMORY_LIMIT;
typedef struct _meta {
    size_t size;
    char is_allocated;
    struct _meta * next;
    struct _meta * prev;
} meta;


typedef struct _btag {
    size_t size;
} btag;


static void * heap_start;
static void * heap_end;
static meta * free_list_head;

void free_list_remove(meta * ptr) {
    //handle case where ptr == head of list
    if (!ptr) return;

    
    if (ptr == free_list_head) {
        free_list_head = free_list_head->next;
        if (free_list_head) free_list_head->prev = NULL;
        return;
    }
    
    meta * prev = ptr->prev;
    meta * next = ptr->next;

    prev->next = next;
    if (next) {
        next->prev = prev;
    }

    ptr->prev = NULL;
    ptr->next = NULL;
}

void free_list_add(meta * ptr) {
    if (!ptr) {
        return;
    }
    
    if (!free_list_head) {
        free_list_head = ptr;
        free_list_head->next = NULL;
        free_list_head->prev = NULL;

        return;
    } 

    ptr->next = free_list_head->next;
    ptr->prev = NULL;
    free_list_head = ptr;

    if (ptr->next) {
        ptr->next->prev = ptr;
    }
}

void * find_first_fit(size_t size) {
   meta * free_iter = free_list_head;

   while (free_iter) {
       if (free_iter->size >= size) {
           free_iter->is_allocated = 1;
           free_list_remove(free_iter);
           return free_iter;
       }

       free_iter = free_iter->next;
   }

   return NULL;
}
/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  return NULL;
}

void * create_new_block(size_t size) {
    if (size == 0) return NULL;

    size_t min_size = size + sizeof(meta) + sizeof(btag);
    void * mem_block = sbrk(min_size);
    
    if (mem_block == (void *) -1) return NULL;

    heap_end = sbrk(0);

    meta * mem_data = (meta *) mem_block;
    mem_data->is_allocated = 1;
    mem_data->size = size;
    mem_data->next = NULL;
    mem_data->prev = NULL;

    btag * mem_btag = (btag *) (mem_block + sizeof(meta) + size);
    mem_btag->size = size;


    return mem_block + sizeof(meta);
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    if (!heap_start) {
        heap_start = sbrk(0);
    }

    void * first_fit = find_first_fit(size);
    if (first_fit) return first_fit;

    return create_new_block(size);
    
}
/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    if (!ptr) return;

    meta * ptr_meta = (meta *) (ptr - sizeof(meta));

    free_list_add(ptr_meta);
    
}   

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    return NULL;
}
