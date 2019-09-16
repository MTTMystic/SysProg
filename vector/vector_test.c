/**
 * Vector
 * CS 241 - Fall 2019
 */
#include "vector.h"
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // Write your test cases here
    vector * v = vector_create(int_copy_constructor, int_destructor, int_default_constructor);
    
    //test size of empty vector
    assert(vector_size(v) == 0);

    //capacity of initial vector?
    assert(vector_capacity(v) == INITIAL_CAPACITY);

    //vector should be empty
    assert(vector_empty(v));

    //add element (not beyond capacity)
    int * example = malloc(sizeof(int));
    *example = 5;
    vector_push_back(v, example);
    
    //was size incremented?
    assert(!vector_empty(v));
    assert(vector_size(v) == 1);

    //check front & end
    void ** first_elem = vector_begin(v);
    assert(*first_elem == example); //verifies that first void * in vector is addr of example

    void ** last_elem = vector_end(v);
    last_elem--;
    assert(*last_elem == example);
    
    //check that both front and end still work if more than one element exists in vector
    int * example_1 = malloc(sizeof(int));
    *example_1 = 9;
    vector_push_back(v, example_1);

    first_elem = vector_begin(v);
    last_elem = vector_end(v);

    assert(*first_elem == example);
    last_elem--;
    assert(*last_elem == example_1);

    //checking "resize smaller"
    vector_resize(v, 1);

    assert(vector_size(v) == 1);
    assert(*(vector_begin(v)) == example);

    //checking "resize larger (within capacity)"
    vector_resize(v, 5);
    assert(vector_size(v) == 5);
    
    //checking resize upgrade capacity
    vector_resize(v, 9);
    assert(vector_capacity(v) == 16);
    assert(vector_size(v) == 9);

    //checking vector at
    void ** elem_ref = vector_at(v, 4);
    int * elem_ptr = *elem_ref;
    assert(*elem_ptr == 0);

    //checking vector_set
    int * example_2 = malloc(sizeof(int));
    *example_2 = 2;
    vector_set(v, 7, example_2);

    elem_ref = vector_at(v, 7);
    elem_ptr = *elem_ref;
    assert(elem_ptr == example_2);

    vector_pop_back(v);
    assert(vector_size(v) == 8);
    
    vector_insert(v, 7, example_2);
    assert(vector_get(v, 7) == example_2);
    assert(vector_size(v) == 9);

    vector_insert(v, 7, example_1);
    assert(vector_get(v, 7) == example_1);
    assert(*(int *)vector_get(v, 8) == *example_2);
    assert(vector_size(v) == 10);

    vector_erase(v, 7);
    assert(vector_size(v) == 9);
    assert(vector_get(v, 7) == example_2);
    return 0;
}
