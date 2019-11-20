/**
 * Mad Mad Access Patterns
 * CS 241 - Fall 2019
 */
#include "tree.h"
#include "utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

static char * file_addr;


int searchFile(uint32_t offset, char * word);
int get_word(uint32_t node_idx, char * buf);
float get_price(uint32_t node_idx);
uint32_t get_count(uint32_t node_idx);
uint32_t get_left_child(uint32_t node_idx);
uint32_t get_right_child(uint32_t node_idx);

/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses mmap to access the data.

  ./lookup2 <data_file> <word> [<word> ...]
*/

int main(int argc, char **argv) {
    //verify argcount
    if (argc < 3) {
      printArgumentUsage();
      exit(1);
    }

    //get filename 
    char * filename = argv[1];
    
    //try to open the file
    int lookup_fd = open(filename, O_RDONLY);

    //print error message if file doesn't open
    if (lookup_fd < 0) {
      openFail(filename);
      exit(2);
    }

    //find the size of the file
    struct stat f_stat;
    fstat(lookup_fd, &f_stat);
    size_t filesize = f_stat.st_size;

    //use mmap to get file in memory
    file_addr = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, lookup_fd, 0);
    
    //check first 4 bytes of file
    char header[5];
    for (size_t idx = 0; idx < 4; idx++) {
      header[idx] = file_addr[idx];
    }

    header[4] = 0;
    if (strcmp(header, "BTRE")) {
      //exit if header is incorrect
      formatFail(filename);
      munmap(file_addr, filesize);
      close(lookup_fd);
    }

    //search file
    int idx = 2;
    for (; idx < argc; idx++) {
      int result = searchFile(4, argv[idx]);
      if (!result) {
        printNotFound(argv[idx]);
      }
    }

    //unmap and close fd
    munmap(file_addr, filesize);
    close(lookup_fd);

    return 0;
}

int searchFile(uint32_t node_idx, char * word) {
  //get current node word
  char word_buf[255];
  get_word(node_idx, word_buf);

  int comp = strcmp(word_buf, word);

  //are they equal?
  if (!comp) {
    //get price of word
    float price = get_price(node_idx);
    uint32_t count = get_count(node_idx);
    printFound(word, count, price);
    return 1;
  }

  //find children of current node
  uint32_t left_offset = get_left_child(node_idx);
  uint32_t right_offset = get_right_child(node_idx);

  if (comp > 0 && left_offset != 0) {
    return searchFile(left_offset, word);
  } else if (comp < 0 && right_offset != 0) {
    return searchFile(right_offset, word);
  }

  return 0;

}

int get_word(uint32_t node_idx, char * buf) {
    //get idx of word
    int word_idx = node_idx + 16;
    int word_iter = 0;
    char c;

    //loop until null terminator found
    while ((c = file_addr[word_idx + word_iter]) != 0) {
      buf[word_iter] = c;
      word_iter++;
    }

    //add null terminator
    buf[word_iter] = 0;

    return word_iter;
}

float get_price(uint32_t node_idx) {
  int price_idx = node_idx + 12;
  float price = *(float *)(file_addr + price_idx);
  return price;
}

uint32_t get_count(uint32_t node_idx) {
  int count_idx = node_idx + 8;
  uint32_t count =  *(uint32_t *)(file_addr + count_idx);
  return count;
  //return 0;
}

uint32_t get_left_child(uint32_t node_idx) {
  uint32_t left_offset = *(uint32_t *)(file_addr + node_idx);
  return left_offset;
}

uint32_t get_right_child(uint32_t node_idx) {
  int right_idx = node_idx + 4;
  uint32_t right_offset = *(uint32_t *)(file_addr + right_idx);
  return right_offset;
}