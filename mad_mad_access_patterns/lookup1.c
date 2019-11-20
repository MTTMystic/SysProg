/**
 * Mad Mad Access Patterns
 * CS 241 - Fall 2019
 */
#include "tree.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

static FILE * lookup_file;

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

int searchFile(uint32_t offset, char * word);
int get_word(uint32_t node_posn, char * word_buf);
float get_price(uint32_t node_posn);
uint32_t get_count(uint32_t node_posn);
uint32_t get_left_child(uint32_t node_posn);
uint32_t get_right_child(uint32_t node_posn);


int main(int argc, char **argv) {
    if (argc < 3) {
      printArgumentUsage();
      exit(1);
    }

    //get filename for lookup
    char * filename = argv[1];
    
    //try to open file
    lookup_file = fopen(filename, "r");

    //print error message if file does not open
    if (!lookup_file) {
      openFail(filename);
      exit(2);
    }

    //check first 4 bytes of file
    char header[5];
    int bytes_read = fread(header, 1, 4, lookup_file);
    header[bytes_read] = 0;

    //exit on incorrectly formatted/unreadable file
    if (strcmp(header, "BTRE") || bytes_read < 4) {
      formatFail(filename);
      fclose(lookup_file);
      exit(2);
    }

    int arg_idx = 2;
    for (; arg_idx < argc; arg_idx++) {
      int result = searchFile(4, argv[arg_idx]);
      if (!result) {
        printNotFound(argv[arg_idx]);
      }
    }

    fclose(lookup_file);
    return 0;
}

int searchFile(uint32_t offset, char * word) {
  int seek_retval = fseek(lookup_file, offset, SEEK_SET);
  if (seek_retval != 0) {
    return 0;
  }

  
  //get current node word
  char word_buf[255];
  get_word(offset, word_buf);
  
  int comp = strcmp(word_buf, word);
  //are they equal?
  if (!comp) {
    //get the price of the word
    float price = get_price(offset);
    uint32_t count = get_count(offset);
    printFound(word, count, price);
    return 1;
  }

  //find children of current node
  uint32_t left_offset = get_left_child(offset);
  uint32_t right_offset = get_right_child(offset);

  if (comp > 0 && left_offset != 0) {
    return searchFile(left_offset, word);
  } else if (comp < 0 && right_offset != 0) {
    return searchFile(right_offset, word);
  }

  return 0;
}

int get_word(uint32_t node_posn, char * word_buf) {
  if (node_posn == 0) {
    return -1;
  }

  //seek to word of current node
  int word_posn = node_posn + 16;
  fseek(lookup_file, word_posn, SEEK_SET);
  
  int idx = 0;
  char c;

  while ((c = fgetc(lookup_file)) != 0) {
    word_buf[idx] = c;
    idx++;
  }

  //add null-terminating byte
  word_buf[idx] = 0;

  //seek to original position of file
  fseek(lookup_file, node_posn, SEEK_SET);

  return idx;
}

float get_price(uint32_t node_posn) {
  
  int price_posn = node_posn + 12;
  fseek(lookup_file, price_posn, SEEK_SET);

  float price = 0;

  fread((char *) &price, 4, 1, lookup_file);
  fseek(lookup_file, node_posn, SEEK_SET);
  
  return price;
}

uint32_t get_count(uint32_t node_posn) {
  int count_posn = node_posn + 8;
  fseek(lookup_file, count_posn, SEEK_SET);
  
  uint32_t count = 0;

  fread((char *) &count, 4, 1, lookup_file);
  fseek(lookup_file, node_posn, SEEK_SET);

  return count;
}

uint32_t get_left_child(uint32_t node_posn) {
  uint32_t left_offset = 0;
  fread((char *) &left_offset, 4, 1, lookup_file);
  return left_offset;
}

uint32_t get_right_child(uint32_t node_posn) {
  uint32_t right_posn = node_posn + 4;
  fseek(lookup_file, right_posn, SEEK_SET);
  uint32_t right_offset = 0;
  fread((char *) &right_offset, 4, 1, lookup_file);
  fseek(lookup_file, node_posn, SEEK_SET);
  return right_offset;
}