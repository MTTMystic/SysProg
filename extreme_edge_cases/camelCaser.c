/**
 * Extreme Edge Cases
 * CS 241 - Fall 2019
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
char **camel_caser(const char *input_str) {
    // TODO: Implement me!
   

    int sentence_count = count_sentences(input_str);
    //sentence count + 1 gives # of elements for outer result array
    char ** result = malloc(sizeof(char *) * (sentence_count + 1));
    result[sentence_count] = NULL;

    allocate_sentence_memory(input_str, result);

    int sentence_idx = 0; //which sentence are we at?
    int char_count = 0; //how many characters processed so far (in current sentence)?
    int char_idx = 0;     //which char (in input_str) are we at?
    int is_first_char = 0; //is char first in sentence?
    int should_capitalize_next = 0; //should capitalize next letter?

    
    while(result[sentence_idx]) {
        char c = input_str[char_idx];
        char_idx++;
        
         //need to create new char variable, since c is from const argument and can't be changed
        char camel_cased_char = c;
        
        if (ispunct(c)) {
            sentence_idx++;
            char_count = 0;
            is_first_char = 1;
            should_capitalize_next = 0;
            continue; //skip all following code & go to next iteration
        } else if (isspace(c)) {
            should_capitalize_next = 1;
            continue;
        }
        
        is_first_char = char_count == 0;

        if (isalpha(c)) {
            should_capitalize_next &= (!is_first_char);
            camel_cased_char = should_capitalize_next ? toupper(c) : tolower(c); //using tolower removes capitals in wrong place
        } 
        
        //putchar(camel_cased_char);
        should_capitalize_next = 0;
        is_first_char = 0;
        result[sentence_idx][char_count] = camel_cased_char;
        char_count++;
        
    }
    
    return result;
}

int count_sentences(const char * input_str) {
    int sentence_count = 0;
    int char_idx;
    for (char_idx = 0; input_str[char_idx] != '\0'; char_idx++) {
        if (ispunct(input_str[char_idx])) {
            sentence_count++;
        }
    }

    return sentence_count;
}

int allocate_sentence_memory(const char * input_str, char ** result) {
    int char_count = 0; 
    int sentence_idx = 0;

    int char_idx;
    for(char_idx = 0; input_str[char_idx] != '\0'; char_idx++) {
        if (ispunct(input_str[char_idx])) {
            result[sentence_idx] = malloc(char_count + 1); //add 1 to fit null terminator
            result[sentence_idx][char_count] = '\0';
            char_count = 0;
            sentence_idx++;
        } //do not count spaces as chars, since they will not be included in final result
        
        else if (!isspace(input_str[char_idx])) {
            char_count++;
        }
    }

    return char_count > 0;
}

void destroy(char **result) {
    // TODO: Implement me!
    size_t result_size = sizeof(result) / sizeof(char *);
    size_t idx = 0;
    for(; idx < result_size; idx++) {
        free(result[idx]);
        result[idx] = NULL;
    }

    free(result);
    result = NULL;
    return;
}
