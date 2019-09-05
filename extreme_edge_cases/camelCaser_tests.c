/**
 * Extreme Edge Cases
 * CS 241 - Fall 2019
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "camelCaser.h"
#include "camelCaser_tests.h"


int assert_equals(char ** actual_output, char * expected_output[]) {

    size_t idx;
    int success = 1; 
    for (idx = 0; actual_output[idx] != NULL; idx++) {
        if (!strcmp(actual_output[idx], expected_output[idx])) {
            printf("Elements at idx %d match\n", (int) idx);
        } else {
            printf("Expected \"%s\" but result was \"%s\" \n", expected_output[idx], actual_output[idx]);
            success = 0;
        }
    }

    success &= (actual_output[idx] == NULL);

    return success;
}

int test_input(char **(*camelCaser)(const char *), void (*destroy)(char **),
                char * input_str, char * expected[]) {
    char ** actual_output = camelCaser(input_str);
    int success = assert_equals(actual_output, expected);
    destroy(actual_output);
    return success;
}
/*
 * Testing function for various implementations of camelCaser.
 *
 * @param  camelCaser   A pointer to the target camelCaser function.
 * @param  destroy      A pointer to the function that destroys camelCaser
 * 			output.
 * @return              Correctness of the program (0 for wrong, 1 for correct).
 */
int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **)) {
    // TODO: Return 1 if the passed in function works properly; 0 if it doesn't.

    int success = 0;
    char * expected_0[] = {"all", "punctuation", "is", "recognized", NULL};
    success = test_input(camelCaser, destroy, "All? Punctuation! Is. Recognized,", expected_0);


    char * expected_1[] = {"", NULL};
    success = test_input(camelCaser, destroy, "   .    ", expected_1);

    return success;
    
}
