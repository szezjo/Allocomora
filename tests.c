#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "allocomora.h"
#include "custom_unistd.h"

int main() {
    int int_status;
    
    printf("* Test 1: initialization of the heap :: ");
    if(LOG || TESTING) printf("\n");
    int_status = heap_setup();
    assert(int_status==0);
    if(LOG || TESTING) printf("* Test 1 :: ");
    printf("SUCCESS!\n");

    size_t init_free_bytes = heap_get_free_space();
    size_t init_used_bytes = heap_get_used_space();

    printf("* Test 2: allocating a 400KB chunk :: ");
    if(LOG || TESTING) printf("\n");
    char *p1 = heap_malloc(400);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==400);
    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    if(LOG || TESTING) printf("* Test 2 :: ");
    printf("SUCCESS!\n");

    printf("* Test 3: allocating a chunk that needs extending a space :: ");
    if(LOG || TESTING) printf("\n");
    size_t size = (PAGES_BGN*PAGE_SIZE)+100;
    p1 = heap_malloc(size);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==size);
    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_space()==init_used_bytes);
    assert(heap_get_free_space()==PAGE_SIZE*get_heap()->pages-heap_get_used_space());
    if(LOG || TESTING) printf("* Test 3 :: ");
    printf("SUCCESS!\n");

    printf("* Test 4: resetting a heap :: ");
    if(LOG || TESTING) printf("\n");
    int_status=heap_reset();
    assert(int_status==0);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_space()==init_used_bytes);
    assert(heap_get_free_space()==init_free_bytes);
    if(LOG || TESTING) printf("* Test 4 :: ");
    printf("SUCCESS!\n");

    printf("* Test 5: trying to allocate too big chunk, it shall not pass :: ");
    if(LOG || TESTING) printf("\n");
    p1 = heap_malloc(100*MB);
    assert(p1==NULL);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_space()==init_used_bytes);
    assert(heap_get_free_space()==init_free_bytes);
    if(LOG || TESTING) printf("* Test 5 :: ");
    printf("SUCCESS!\n");

    printf("* Test 6: reallocating a chunk - new size is bigger, next chunk is free :: ");
    if(LOG || TESTING) printf("\n");
    p1 = heap_malloc(250);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==250);
    char *p2 = heap_realloc(p1,500);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p2)->size==500);
    heap_free(p2);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    if(LOG || TESTING) printf("* Test 6 :: ");
    printf("SUCCESS!\n");

    printf("* Test 7: reallocating a chunk - new size is smaller :: ");
    if(LOG || TESTING) printf("\n");
    p1 = heap_malloc(250);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==250);
    p2 = heap_realloc(p1,120);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p2)->size==120);
    heap_free(p2);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    if(LOG || TESTING) printf("* Test 7 :: ");
    printf("SUCCESS!\n");

    printf("* Test 8: reallocating a chunk - new size is bigger, but next chunk is not free :: ");
    if(LOG || TESTING) printf("\n");
    p1 = heap_malloc(250);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==250);
    p2 = heap_malloc(400);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==2);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p2)->size==400);
    char *p3 = heap_realloc(p1,500);
    assert(p3!=NULL);
    assert(get_pointer_type(p3)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==2);
    assert(heap_get_free_gaps_count()==2);
    assert(heap_get_control_block(p3)->size==500);
    heap_free(p2);
    heap_free(p3);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    if(LOG || TESTING) printf("* Test 8 :: ");
    printf("SUCCESS!\n");

    printf("* Test 9: testing realloc with memblock being NULL, size being 0 (free) or size being unchanged :: ");
    if(LOG || TESTING) printf("\n");
    p1 = heap_realloc(NULL,250);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==250);
    p2 = heap_realloc(p1,250);
    assert(p1==p2);
    p3 = heap_realloc(p1,0);
    assert(p3==NULL);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    if(LOG || TESTING) printf("* Test 9 :: ");
    printf("SUCCESS!\n");

    printf("* Test 10: testing calloc :: ");
    if(LOG || TESTING) printf("\n");
    p1 = heap_calloc(4,100);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==4*100);
    for(int i=0; i<4*100; i++) {
        assert(*(p1+i)==0);
    }
    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    if(LOG || TESTING) printf("* Test 10 :: ");
    printf("SUCCESS!\n");

    printf("* Test n: description :: ");
    if(LOG || TESTING) printf("\n");
    if(LOG || TESTING) printf("* Test n :: ");
    printf("SUCCESS!\n");

    printf("* Test n: description :: ");
    if(LOG || TESTING) printf("\n");
    if(LOG || TESTING) printf("* Test n :: ");
    printf("SUCCESS!\n");

    heap_delete(0);
    assert(get_heap()->is_set==0);
    return 0;
}