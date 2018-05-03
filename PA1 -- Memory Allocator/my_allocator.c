/* 
    File: my_allocator.c

    Author: Oneal Abdulrahim
            Department of Computer Science
            Texas A&M University
    Date  : 02-05-2018

    Modified: 

    This file contains the implementation of the module "MY_ALLOCATOR".

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "my_allocator.h"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

char * MEMORY_POOL;


/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/
    
int MEMORY_SIZE;
int BASIC_BLOCK_SIZE;
int REAL_BLOCK_SIZE;

block ** FREE_LIST; // free list is a pointer array

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FUNCTIONS FOR MODULE MY_ALLOCATOR */
/*--------------------------------------------------------------------------*/

// helper functions
bool is_power_of_two(ulong a) {
    return (a & (a - 1)) == 0;
}

/*
This method first ensures the proper input is given to the function, such that
the basic block size is smaller than the amount of memory needed, as well as 
both being a power of 2 (per specs).

Allocates memory by setting the header pointer to the first memory location

Sets up free lists as pointer array (linked lists containing free blocks)
*/
unsigned int init_allocator(unsigned int _basic_block_size, unsigned int _length) {
    // validate input
    if (_basic_block_size > _length
      || !is_power_of_two(_basic_block_size) && !is_power_of_two(_length)) {
        return 0;
    }

    // allocate the memory
    BASIC_BLOCK_SIZE = _basic_block_size;
    REAL_BLOCK_SIZE = (int) (log(BASIC_BLOCK_SIZE) / log(2));
    MEMORY_SIZE = _length;
    MEMORY_POOL = (char *) (calloc(MEMORY_SIZE, 1)); // initializes memory region

    // set up the free list -- allocate the proper size needed by power of 2
    FREE_LIST = (block **) (calloc((log(MEMORY_SIZE) / log(2)) - REAL_BLOCK_SIZE + 1, sizeof(block*)));
    block * head = (block * ) MEMORY_POOL; // ptr to beginning of memory pool
    head -> next_block = NULL; // not split up yet
    head -> size = MEMORY_SIZE; // all of the memory is available
    head -> is_free = 0; // 0 means it's not occupied

    // now, this block is updated in the "free lists" pointer array
    FREE_LIST[(int) (log(MEMORY_SIZE) / log(2)) - REAL_BLOCK_SIZE] = head;
    
    // return the memory size that has been allocated
    return head -> size;
}

/* 
All memory is freed in this function, and the memory pool returns to nullptr
*/ 
int release_allocator() {
    free(MEMORY_POOL);
    free(FREE_LIST);
    MEMORY_POOL = NULL;
    return 0;
}

// splits and makes a new block
void split_block(block* h, int available_memory) {
    // Now, split blocks, delete current block header, setup headers, insert
    block * buddy = (block *) ((char *) (h) + available_memory / 2);
    buddy -> next_block = h -> next_block;
    buddy -> size = available_memory / 2;
    buddy -> is_free = 0;
    h -> next_block = buddy;
    h -> size = available_memory / 2;
    h -> is_free = 0;
    
    // order matters during insertion!!
    FREE_LIST[(int) (log(available_memory / log(2)) - REAL_BLOCK_SIZE)] = buddy -> next_block;
    available_memory /= 2; // since we split the blocks!
    FREE_LIST[(int) (log(available_memory / log(2)) - REAL_BLOCK_SIZE)] = h;
    
    buddy -> next_block = NULL;
}

extern Addr my_malloc(unsigned int _length) {
    // make sure we have some memory to give
    if (MEMORY_POOL == NULL) {
        printf("All memory released... no memory to allocate");
        return NULL;
    }

    // calculate sizing -- make sure we have enough!
    int needed_memory = REAL_BLOCK_SIZE;

    // determine needed block size
    while (true) {
        if (needed_memory >= _length + 16) { // accomodate overhead
            break;
        }

        needed_memory *= 2;
    }

    int available_memory = needed_memory; // for now

    // How much memory is available to use?
    // Updates made to available_memory
    while (true) {
        if (available_memory > MEMORY_SIZE) {
            printf("ERROR: OUT OF MEMORY! NOT ENOUGH AVAILABLE"); // exception
            return NULL; // fail -- function is exited
        } else if (FREE_LIST[(int) (log(available_memory / log(2)) - REAL_BLOCK_SIZE)] != NULL) {
            break; // if there's enough to accomodate -- the current memory size is available
        }   
        available_memory *= 2;
    }

    while (true) {
        // if we need to use ALL of the memory, no need to split
        if (available_memory == needed_memory) {
            block * current = FREE_LIST[(int) (log(available_memory / log(2)) - REAL_BLOCK_SIZE)];
            FREE_LIST[(int) (log(available_memory / log(2)) - REAL_BLOCK_SIZE)] = current -> next_block;
            current -> is_free = 1; // OCCUPIED
            current -> next_block = NULL;
        } else { // now we are ready to split blocks!! it's needed
            split_block(FREE_LIST[(int) (log(available_memory / log(2)) - REAL_BLOCK_SIZE)]
                      , available_memory);
        }
    }
    
}

void combine_blocks() {
    // needs implementation
    // delet two small blocks, merge into big blocks from free list
    // assign the small address to large block header
    // add large block back to free list
}

/*
add block back to free list, merge if possible
*/
extern int my_free(Addr _a) {
    block * h = (block *) (char *) _a - 16; // formula from slides
    h -> is_free = 0; // it's freed now
    if (FREE_LIST[(int) (log((h -> size) / log(2)) - REAL_BLOCK_SIZE)] == NULL) {
        FREE_LIST[(int) (log((h -> size) / log(2)) - REAL_BLOCK_SIZE)] = h;
    } else { // if not null (occupuied)
        memset(MEMORY_POOL, 0, sizeof(MEMORY_POOL)); // zero it out
        block * header = (block *) MEMORY_POOL;
        header -> next_block = NULL;
        header -> size = MEMORY_SIZE;
        header -> is_free = 0;

        FREE_LIST[(int) (log(MEMORY_SIZE / log(2)) - REAL_BLOCK_SIZE)] = header;
    }

    combine_blocks();
}

