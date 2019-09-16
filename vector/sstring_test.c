/**
 * Vector
 * CS 241 - Fall 2019
 */
#include "sstring.h"
#include <assert.h>
#include <string.h>


int main(int argc, char *argv[]) {
    sstring * example = cstr_to_sstring("wow");
    char * converted_back = sstring_to_cstr(example);
    assert(strlen(converted_back) == 3);
    assert(!strcmp(converted_back, "wow"));

    sstring * another = cstr_to_sstring(" caleb ily");
    int new_len = sstring_append(example, another);
    assert(new_len == 13);

    sstring * empty = cstr_to_sstring("");
    new_len = sstring_append(example, empty);
    assert(new_len == 13);

    vector * split_string = sstring_split(another, ' ');
    assert(vector_size(split_string) == 2);
    return 0;
}
