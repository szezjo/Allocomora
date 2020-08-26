#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "allocomora.h"
#include "custom_unistd.h"

int main() {
    int int_status;
    
    printf("* Test 1: initialization of the heap\n");
    int_status = heap_setup();
    assert(int_status==0);
    printf("* Test 1: success!\n");

    size_t init_free_bytes = heap_get_free_space();
    size_t init_used_bytes = heap_get_used_space();

    printf("* Test 2: allocating a 400KB chunk\n");
    char *p1 = heap_malloc(400);
    assert(p1!=NULL);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    struct chunk_t *p1_chunk = heap_get_control_block(p1);
    assert(p1_chunk->size==400);
    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    printf("* Test 2: success!\n");

    printf("* Test 3: allocating a chunk that needs extending a space\n");
    size_t size = (PAGES_BGN*PAGE_SIZE)+100;
    p1 = heap_malloc(size);
    assert(p1!=NULL);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    p1_chunk = heap_get_control_block(p1);
    assert(p1_chunk->size==size);
    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_space()==init_used_bytes);
    assert(heap_get_free_space()==PAGE_SIZE*get_heap()->pages-heap_get_used_space());
    printf("* Test 3: success!\n");

    printf("* Test 4: resetting a heap\n");
    int_status=heap_reset();
    assert(int_status==0);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_space()==init_used_bytes);
    assert(heap_get_free_space()==init_free_bytes);
    printf("* Test 4: success!\n");

    printf("* Test 5: trying to allocate too big chunk, it shall not pass\n");
    p1 = heap_malloc(100*MB);
    assert(p1==NULL);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    printf("* Test 5: success!\n");

    printf("* Test 6: ")
    


    heap_delete(0);
    return 0;
}