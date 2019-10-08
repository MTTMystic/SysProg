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
    if (!ptr) return;

    ptr->is_allocated = 1; //removing from free list -> block allocated

    meta * prev = ptr->prev; //save payload values
    meta * next = ptr->next;

    ptr->prev = NULL;
    ptr->next = NULL;

    //handle case where ptr == head of list
    if (ptr == free_list_head) {
        meta * new_flh = free_list_head->next;
        
        free_list_head = new_flh;
        if (free_list_head)
            free_list_head->prev = NULL; //ensure first node has no prev value
        return;
    }

    
    if (prev) 
        prev->next = next; //join neighboring nodes in linked list
    if (next)
        next->prev = prev;
}   

void free_list_add(meta * ptr) {
    if (!ptr) return;

    ptr->is_allocated = 0; //adding to free list -> block not alloc'd
     
    if (!free_list_head) {
        ptr->prev = NULL; ptr->next = NULL;
        free_list_head = ptr;
        return;
    }

    ptr->next = free_list_head;
    free_list_head->prev = ptr;
    ptr->prev = NULL;
    free_list_head = ptr;
}

void * split_block(size_t request_size, meta * ptr) {
    if (!ptr) return NULL;

    size_t old_size = ptr->size;
    size_t leftover_bytes = old_size - request_size - sizeof(meta) - sizeof(btag);

    //handle case where there's not enough room for another block + overhead
    //cons: wasteful, gives user back more space than they requested
    //pros: avoid headache of untagged memory blocks
    if (leftover_bytes <= 0) {
        if (!ptr->is_allocated) {
            free_list_remove(ptr); //if this pointer was free, remove it from free list
        }
        return ptr; 
    }

    void * void_ptr = (void *) ptr;
    ptr->size = request_size;
    btag * ptr_btag = void_ptr + sizeof(meta) + request_size;
    ptr_btag->size = request_size;

    meta * leftover_meta = (meta *) (void_ptr + sizeof(meta) + ptr->size + sizeof(btag));
    leftover_meta->size = leftover_bytes;

    btag * leftover_btag = (btag *) ((void *) leftover_meta + sizeof(meta) + leftover_bytes);
    leftover_btag->size = leftover_bytes;

    if (!ptr->is_allocated) {
        ptr->is_allocated = 1;
    
        leftover_meta->next = ptr->next;
        leftover_meta->prev = ptr->prev;

        if (leftover_meta->next) {
            leftover_meta->next->prev = leftover_meta;
        }

        if (leftover_meta->prev) {
            leftover_meta->prev->next = leftover_meta;
        }

        ptr->prev = NULL; ptr->next = NULL;
    } else {
        free_list_add(leftover_meta);
    }

    return void_ptr + sizeof(meta);

}

void * find_first_fit(size_t size) {
   meta * free_iter = free_list_head;

   while (free_iter) {
       if (free_iter->size >= size) {
           return split_block(size, free_iter);
       }

       free_iter = free_iter->next;
   }

   return NULL;
}

/*
    Coalesce with right neighbor.
    1) Check that there is a succeeding mem block (this node is not last before heap end)
    2) Check this succeeding block is free.
    3) If the successor is free, it is already a part of the free list.
    4) Using LIFO insertion policy, no guarantee that nodes in free list are address-ordered
        a) how to maintain list without knowing how ptrs should be updated?
        b) remove succeeding block from free list, join the two blocks, then add larger block
        c) (from https://courses.cs.washington.edu/courses/cse351/17sp/lectures/CSE351-L24-memalloc-II_17sp-ink-day2.pdf)
*/
int coalesce_r(meta * ptr) {
    if (!ptr) return 0;

    heap_end = sbrk(0);
    //check that there is a next block
    meta * rn_meta = (meta *) ((void *) ptr + sizeof(meta) + ptr->size + sizeof(btag));

    if ((void *) rn_meta >= heap_end) return 0;

    //check that next block is free
    if (rn_meta->is_allocated) return 0;

    //remove next block from free list
    free_list_remove(rn_meta);

    //btag * ptr_btag = (btag *) ((void *) ptr + sizeof(meta) + ptr->size);

    //join blocks
    size_t new_size = ptr->size + sizeof(btag) + sizeof(meta) + rn_meta->size;
    ptr->size = new_size;

    btag * rn_btag = (btag *) ((void *) ptr + sizeof(meta) + ptr->size);
    rn_btag->size = new_size;

    //add block back to free list
    free_list_add(rn_meta);

    return 1;
}

/*
    Coalesce with left neighbor.
    1) Check that there is a preceding mem block (this node is not last before heap end)
    2) Check this preceding block is free.
    3) If the successor is free, it is already a part of the free list.
    4) Using LIFO insertion policy, no guarantee that nodes in free list are address-ordered
        a) how to maintain list without knowing how ptrs should be updated?
        b) remove succeeding block from free list, join the two blocks, then add larger block
        c) (from https://courses.cs.washington.edu/courses/cse351/17sp/lectures/CSE351-L24-memalloc-II_17sp-ink-day2.pdf)
*/
int coalesce_l(meta * ptr) {
    if (!ptr) return 0;

    //check that there is a previous block
    if ((void *) ptr == heap_start) return 0;

    btag * ln_btag = (btag *) ((void *) ptr - sizeof(btag));
    meta * ln_meta = (meta *) ((void *) ptr - sizeof(btag) - ln_btag->size - sizeof(meta));
    //check preceding block is free
    if (ln_meta->is_allocated) return 0;

    //remove preceding block from free list
    free_list_remove(ln_meta);

    //join the two blocks
    size_t new_size = ln_meta->size + sizeof(btag) + sizeof(meta) + ptr->size;
    ln_meta->size = new_size;

    ln_btag = (btag *) ((void *) ln_meta + sizeof(meta) + ln_meta->size);
    ln_btag->size = new_size;

    //add larger block back to free list
    free_list_add(ln_meta);

    return 1;
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
    int did_add = coalesce_l(ptr_meta) || coalesce_r(ptr_meta); 
    if (!did_add) {
        free_list_add(ptr_meta);
    }
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
    if (!ptr) {
        return malloc(size);
    }
    
    if (!size) {
        free(ptr);
        return NULL;
    }

    meta * ptr_meta = (meta *) (ptr - sizeof(meta));

    if (ptr_meta->size >= size) {
        return split_block(size, ptr_meta);
    }
    
    void * ret_ptr = malloc(size);

    if (ret_ptr) {
        memmove(ret_ptr, ptr, ptr_meta->size);
    }
    
    return ret_ptr;
}
