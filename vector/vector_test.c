/**
 * Vector
 * CS 241 - Fall 2019
 */
#include "vector.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Write your test cases here
    vector * v = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    assert(vector_empty(v) && vector_size(v) == 0);

    vector_push_back(v, "one");
    vector_push_back(v, "");
    vector_push_back(v, "huh");
    vector_push_back(v, NULL);
    vector_push_back(v, "\n");
    vector_push_back(v, "caleb");
    vector_push_back(v, "heart");
    
    assert(vector_size(v) == 7);

    assert(!strcmp(*vector_front(v), "one"));
    assert(!strcmp(*vector_back(v), "heart"));

    //vector get?
    void * empty_str = vector_get(v, 1);
    assert(!strcmp(empty_str, ""));
    //empty_str = vector_get(v, 70);
    //set the value at idx 1
    vector_set(v, 1, "not empty");
    empty_str = vector_get(v, 1);
    assert(strcmp(empty_str, ""));

    //resize
    vector_resize(v, 9);
    
    assert(vector_size(v) == 9);
    assert(vector_capacity(v) == 16);
    //printf("elem at idx 7 is: %s\n", vector_get(v, 7));
    assert(!strcmp(vector_get(v, 7), ""));
    assert(!strcmp(vector_get(v, 8), ""));

    //resize smaller

    vector_resize(v, 7);
    assert(vector_size(v) == 7);
    assert(!strcmp(vector_get(v, 6), "heart"));

    //pop back
    vector_pop_back(v);
    assert(vector_size(v) == 6);
    assert(!strcmp(*vector_back(v), "caleb"));

    //insert at idx 2
    vector_insert(v, 2, "oof");
    assert(vector_size(v) == 7);
    assert(!strcmp(*vector_back(v), "caleb"));
    assert(!strcmp(vector_get(v, 2), "oof"));
    assert(!strcmp(vector_get(v, 3), "huh"));

    //insert multiple times
    vector_insert(v, 3, "calico"); //size 8
    vector_insert(v, 3, "tortoiseshell"); //size 9
    vector_insert(v, 4, "tabby"); //size 10
    vector_insert(v, 4, "hairless"); //size 11


    assert(vector_size(v) == 11);
    assert(!strcmp(vector_get(v, 4), "hairless"));
    assert(!strcmp(vector_get(v, 5), "tabby"));
    assert(!strcmp(vector_get(v, 3), "tortoiseshell"));
    assert(!strcmp(vector_get(v, 6), "calico"));

    //erase
    vector_erase(v, 4);

    assert(vector_size(v) == 10);
    assert(!strcmp(vector_get(v, 4), "tabby"));
    assert(!strcmp(vector_get(v, 3), "tortoiseshell"));

    vector_clear(v);
    vector_destroy(v);
}
