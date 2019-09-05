/**
 * Extreme Edge Cases
 * CS 241 - Fall 2019
 */
#include <stdio.h>
#include <stdlib.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"

int main() {
    char * example = "This is an example string. It has sentences. Wow";
    char ** camelCased = camel_caser(example);
    //printf("the last value is %p", camelCased[2]);
    // Feel free to add more test cases of your own!
    /*if (test_camelCaser(&camel_caser, &destroy)) {
        printf("SUCCESS\n");
    } else {
        printf("FAILED\n");
    }*/
}
