/**
 * Extreme Edge Cases
 * CS 241 - Fall 2019
 */
#pragma once

/**
 * Given a string, returns a string with all whitespace removed (other letters intact).
 * */
char * remove_whitespace(char * input_str);

/**
 * Given a string and returns array of punctuation-separated substrings contained within
 * */
char ** extract_sentences(const char * input_str);
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
