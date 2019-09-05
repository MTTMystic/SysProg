/**
 * Extreme Edge Cases
 * CS 241 - Fall 2019
 */
#pragma once

/*
    Counts the number of sentences (words separated by punctuation marks) in input string.
*/
int count_sentences(const char *input_str);

/**
 * Takes input string and:
 * 1) counts characters in each sentence (marked by punctuation mark)
 * 2) allocates enough memory for each sentence and stores pointers to memory in result arr
*/
int allocate_sentence_memory(const char * input_str, char ** result);

/**
 * Takes an input string, and returns the camelCased output. Details on this
 * function, as well as the formal description of camelCase are available in
 * the assignment's documentation
 */
char **camel_caser(const char *input_str);

/**
 * Destroys the camelCased output returned by the camel_caser function
 */
void destroy(char **result);
