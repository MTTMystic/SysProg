/**
 * Perilous Pointers
 * CS 241 - Fall 2019
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);

    int val = 132;
    
    second_step(&val);

    val = 8942;

    int * val_arr[1] = {&val};

    double_step(val_arr);

    empty_step("abc");

    two_step("beautiful", "beautiful");

    char * seq = malloc(6);
    seq[0] = 'a';
    seq[2] = 'a';
    seq[3] = 'a';

    three_step(seq, (seq+2), (seq+4));
    free(seq);

    step_step_step("a@", "aaH", "aaaP");

    it_may_be_odd("a", 97);

    char * immutable = "CS233,CS241,CS357";
    char * mutable = strdup(immutable);

    tok_step(mutable);
    return 0;
}
