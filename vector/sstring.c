/**
 * Vector
 * CS 241 - Fall 2019
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <assert.h>



struct sstring { 
    vector * string;
};

sstring *cstr_to_sstring(const char *input) {
    assert(input);
    //allocate heap memory for sstring
    sstring * new_sstring = malloc(sizeof(sstring));
    

    //create vector using char handlers
    new_sstring->string = char_vector_create();

    vector * v = new_sstring->string;

    size_t idx = 0;
    for (; input[idx] != '\0'; idx++) {
        char current = input[idx];
        vector_push_back(v, &current);
    }
    
    return new_sstring;
}

char *sstring_to_cstr(sstring *input) {
    assert(input);
    vector * string = input->string;
    //requires 1 byte per character + 1 for null terminator
    char * result = malloc(vector_size(string) + 1);
    size_t idx = 0;
    for(; idx < vector_size(string); idx++) {
        void * char_ptr = vector_get(string, idx);
        char current = * (char *) char_ptr;

        result[idx] = current;

    }

    result[sizeof(result) - 1] = '\0';
    return result;
}

int sstring_append(sstring *this, sstring *addition) {
    assert(this);
    assert(addition);

    vector * string = this->string;
    vector * other_string = addition->string;

    size_t start_len = vector_size(string); //start length of this string

    size_t other_len = vector_size(other_string); //# of chars to append

    size_t final_len = start_len + other_len;
    
    //ensure vector has enough capacity to store longer string
    vector_reserve(string, final_len);

    size_t idx = 0;
    for (; idx < other_len; idx++) {
        void * target = vector_get(other_string, idx);
        vector_push_back(string, target);
    }

    return final_len;
}

int count_chars(vector * string, const size_t idx, char delim) {
    int char_cnt = 0;
    size_t lidx = idx;
    for(; lidx < vector_size(string); lidx++) {
        char current = *(char *)vector_get(string, lidx);
        if (current == delim) return 0;
        char_cnt++;
    }
    return char_cnt;
}

vector *sstring_split(sstring *this, char delimiter) {
    assert(this);
    vector * split_string = string_vector_create();
    
    vector * src_string = this->string;

    size_t split_idx = 0;
    while (split_idx < vector_size(src_string)) {
        int token_length = count_chars(src_string, split_idx, delimiter);
        token_length = (token_length > 0) ? token_length : 1;
        char token[token_length];

        if (token_length != 1) {
            int lidx = split_idx;
            for (; lidx < token_length; lidx++) {
                char current = *(char *) vector_get(src_string, lidx);
                token[lidx] = current;
            }
        } 
        
        token[token_length - 1] = '\0';
        vector_push_back(split_string, token);

        split_idx += token_length;   
    }

    return split_string;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    return NULL;
}

void sstring_destroy(sstring *this) {
    // your code goes here
}
