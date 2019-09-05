/**
 * Extreme Edge Cases
 * CS 241 - Fall 2019
 */
#pragma once

int test_camelCaser(char **(*camelCaser)(const char *),
                    void (*destroy)(char **));

int assert_equals(char ** actual_output, char * expectedOutput[]);