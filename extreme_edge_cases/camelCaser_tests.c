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
    size_t actual_size = sizeof(*actual_output) / sizeof(char *);
    size_t expected_size = sizeof(*expected_output) / sizeof(char *);

    if (actual_size != expected_size) {
        return 0;
    }

    for (idx = 0; actual_output[idx] != NULL; idx++) {
        if (!strcmp(actual_output[idx], expected_output[idx])) {
            //printf("Elements at idx %d match\n", (int) idx);
        } else {
            //printf("Expected \"%s\" but result was \"%s\" \n", expected_output[idx], actual_output[idx]);
            return 0;
        }
    }

    return (actual_output[idx] == NULL);

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

    //test punctuation
    char * expected_0[] = {"all", "punctuation", "is", "recognized", "see", "loOk", NULL};

    char * expected_1[] = {"", NULL};

    char * expected_2[] = {"justACamelCasedSentence", NULL};

    char * expected_3[] = {"", "nnnnnn103", NULL};

    char * expected_4[] = {"theFunctionWorks", "givenMultipleMulti", "wordSentences", "asItOughtTo"};

    success = test_input(camelCaser, destroy, "All? Punctuation! Is. Recognized, See: Lo\nok;", expected_0);

    success &= test_input(camelCaser, destroy, "   .    ", expected_1);

    success &= test_input(camelCaser, destroy, "", expected_1);

    success &= test_input(camelCaser, destroy, ".", expected_1);
    
    success &= test_input(camelCaser, destroy, "Just a camel cased sentence!", expected_2);

    success &= test_input(camelCaser, destroy, " Just a camel cased sentence  .     ", expected_2);

    success &= test_input(camelCaser, destroy, "jUST A Camel CASED sENTence.", expected_2);

    success &= test_input(camelCaser, destroy, ".nnnnnn103!", expected_3);

    success &= test_input(camelCaser, destroy, "The function works. Given multiple multi-word sentences. As it ought to.", expected_4);
    
    
    return success;
    
}
