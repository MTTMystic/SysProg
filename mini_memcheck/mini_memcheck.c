/**
 * Mini Memcheck
 * CS 241 - Fall 2019
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

meta_data * head;

size_t total_memory_requested;

size_t total_memory_freed;

size_t invalid_addresses;

/**Helper function to insert a new node of metadata at start of list.*/
void insert_node(meta_data * meta_ptr, size_t request_size, const char * filename, void * instruction) {


    meta_ptr->filename = filename;
    meta_ptr->request_size = request_size;
    meta_ptr->instruction = instruction;
    meta_ptr->next = head;

    head = meta_ptr;
}

/**Helper function to free memory and remove metadata node.
 * Payload supposedly matches ptr returned by malloc et al.
 * Metadata should be just before payload (meta_addr = payload - sizeof(meta_data))
 * So for each node compare payload and meta_addr + sizeof(meta_data) to verify that payload is valid
 * If payload found, deallocate memory + linked list removal
 * If not, count it as an invalid address
 */

void remove_node(void * payload) {
    meta_data * prev_node = NULL;
    meta_data * next_node = NULL;
    meta_data * current_node = head;

    while (current_node) {
       
        //go to block of requested memory per above formula
        void * linked_memory = ((void *) current_node) + sizeof(meta_data);

         //set next node ahead of removal
        next_node = current_node->next;

        //compare linked mem to payload
        if (payload == linked_memory) {
            //removing a block from a linked list:
            //prev_node will point to next_node
            //if there is no prev_node current_node must be head, so change head
            if (!prev_node) {
                head = next_node;
            } else {
                prev_node->next = next_node;
            }
            
            //don't change memory freed if removal is called by realloc
            total_memory_freed += current_node->request_size;
            
            free(current_node); //free memory block

            return; //exit function immediately to prevent unnecessary removals/invalid address false alarm
        }

        prev_node = current_node;
        current_node = next_node;
    }

    invalid_addresses++; //if this code is reached, the given ptr wasn't returned by malloc et al.
} 


void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    if (!request_size) return NULL; //consider request for 0 bytes invalid

    size_t bytes_needed = request_size + sizeof(meta_data); //requested bytes + metadata overhead
    void * memory_block = malloc(bytes_needed); 

    if (!memory_block) return NULL; //if malloc failed
    
    meta_data * meta_d = (meta_data *) memory_block; //for linked list insertion
    void * requested_memory = memory_block + sizeof(meta_data); //actual return value

    //using helper function to add node to linked list
    insert_node(meta_d, request_size, filename, instruction);
    //ignore metadata overhead for mem requests
    total_memory_requested += request_size;

    return requested_memory;
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here

    //undefined behavior if either part of request is 0, just return NULL
    if (num_elements == 0 || element_size == 0) return NULL;
    size_t bytes_needed = (num_elements * element_size) + sizeof(meta_data);
    //mini_malloc should handle allocation + node insertion
    void * memory_block = mini_malloc(bytes_needed, filename, instruction);
    if (!memory_block) return NULL;

    //initializing each element to zero
    memset(memory_block, 0, sizeof(memory_block));

    return memory_block;
}


void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    //undefined behavior for null ptr and request size 0
    if (payload == NULL && request_size == 0) return NULL;
    else if (request_size == 0) { //if new size == 0 call free
        mini_free(payload);
        return NULL;
    } else if (payload == NULL) { //if payload DNE allocate mem
        return mini_malloc(request_size, filename, instruction);
    }

    meta_data * prev_node = NULL;
    meta_data * next_node = NULL;
    meta_data * current_node = head;

    while (current_node) {
       
        //go to block of requested memory per above formula
        void * linked_memory = ((void *) current_node) + sizeof(meta_data);

         //set next node ahead of removal
        next_node = current_node->next;

        //compare linked mem to payload
        if (payload == linked_memory) {
            
            size_t original_request_size = current_node->request_size;

            //removing a block from a linked list:
            //prev_node will point to next_node
            //if there is no prev_node current_node must be head, so change head
            if (!prev_node) {
                head = next_node;
            } else {
                prev_node->next = next_node;
            }
            
            void * memory = realloc(current_node, request_size + sizeof(meta_data));
            if (!memory) return NULL;

            if (original_request_size <= request_size) {
                total_memory_requested += (request_size - original_request_size);
            } else {
                total_memory_freed += (original_request_size - request_size); 
            }

            meta_data * meta_d = (meta_data *) memory;
            void * requested_memory = memory + sizeof(meta_data);

            insert_node(meta_d, request_size, filename, instruction);

            return requested_memory;
        }

        prev_node = current_node;
        current_node = next_node;
    }

    invalid_addresses++; //if this code is reached, the given ptr wasn't returned by malloc et al.
    return NULL;

}


void mini_free(void *payload) {
    //bad free if payload == NULL
    if (!payload) {
        invalid_addresses++;
        return;
    }

    //using helper function to delete metadata node and deallocate memory
    remove_node(payload);
}
