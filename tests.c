#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "allocomora.h"
#include "custom_unistd.h"

size_t init_free_bytes;
size_t init_used_bytes;

void extend_size() {
    if(get_heap()->pages<2) {
        if(LOG) printf("-Log- A malloc_aligned test requires 2 pages or more. Using malloc to increase a space.\n");
        char *p1 = heap_malloc(PAGE_SIZE);
        assert(p1!=NULL);
        heap_free(p1);
        assert(get_heap()->pages>=2);
    }
}

void test1() {
    int status = heap_setup();
    assert(status==0);
}

void test2() {
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
}

void test3() {
    size_t size = (PAGES_BGN*PAGE_SIZE)+100;
    char *p1 = heap_malloc(size);
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
}

void test4() {
    int status=heap_reset();
    assert(status==0);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_space()==init_used_bytes);
    assert(heap_get_free_space()==init_free_bytes);
}

void test5() {
    char *p1 = heap_malloc(100*MB);
    assert(p1==NULL);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_space()==init_used_bytes);
    assert(heap_get_free_space()==init_free_bytes);
}

void test6() {
    char *p1 = heap_malloc(250);
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
}

void test7() {
    char *p1 = heap_malloc(250);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==250);
    char *p2 = heap_realloc(p1,120);
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
}

void test8() {
    char *p1 = heap_malloc(250);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==250);
    char *p2 = heap_malloc(400);
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
}

void test9() {
    char *p1 = heap_realloc(NULL,250);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p1)->size==250);
    char *p2 = heap_realloc(p1,250);
    assert(p1==p2);
    char *p3 = heap_realloc(p1,0);
    assert(p3==NULL);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
}

void test10() {
    char *p1 = heap_calloc(4,100);
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
}

void test11() {
    extend_size();
    char *p1 = heap_malloc_aligned(500);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(((intptr_t)p1 & (intptr_t)(PAGE_SIZE-1))==0); 
    assert(heap_get_used_blocks_count()==1);
    assert(heap_get_free_gaps_count()==2);
    assert(heap_get_control_block(p1)->size==500);
    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()+heap_get_used_space()==get_heap()->pages*PAGE_SIZE);
}

void test12() {
    extend_size();
    char *p1 = heap_malloc(PAGE_SIZE-sizeof(struct chunk_t)*2);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_control_block(p1)->size+sizeof(struct chunk_t)==PAGE_SIZE-sizeof(struct chunk_t));
    char *p2 = heap_malloc_aligned(500);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1))==0); 
    assert(heap_get_used_blocks_count()==2);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p2)->size==500);
    assert(heap_get_control_block(p1)->next==heap_get_control_block(p2));
    heap_free(p1);
    heap_free(p2);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()+heap_get_used_space()==get_heap()->pages*PAGE_SIZE);
}

void test13() {
    extend_size();
    char *p1 = heap_malloc(PAGE_SIZE-sizeof(struct chunk_t)*2);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_control_block(p1)->size+sizeof(struct chunk_t)==PAGE_SIZE-sizeof(struct chunk_t));
    char *p2 = heap_malloc(500);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_control_block(p2)->size==500);
    char *p3 = heap_malloc(100);
    assert(p3!=NULL);
    assert(get_pointer_type(p3)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_control_block(p3)->size==100);
    heap_free(p2);
    assert(heap_get_used_blocks_count()==2);
    assert(heap_get_free_gaps_count()==2);

    p2 = heap_malloc_aligned(500);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(((intptr_t)p2 & (intptr_t)(PAGE_SIZE-1))==0); 
    assert(heap_get_used_blocks_count()==3);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_control_block(p2)->size==500);
    assert(heap_get_control_block(p1)->next==heap_get_control_block(p2));
    assert(heap_get_control_block(p2)->next==heap_get_control_block(p3));
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()+heap_get_used_space()==get_heap()->pages*PAGE_SIZE);
}

void test14() {
    extend_size();
    char *p1 = heap_malloc((get_heap()->pages-1)*PAGE_SIZE);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);
    assert(heap_get_free_space()<PAGE_SIZE);
    char *p2 = heap_malloc_aligned(PAGE_SIZE);
    assert(p2==NULL);
    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()+heap_get_used_space()==get_heap()->pages*PAGE_SIZE);
}

int main() {
    int int_status;
    
    printf("* Test 1: initialization of the heap :: ");
    if(LOG || TESTING) printf("\n");
    test1();
    if(LOG || TESTING) printf("* Test 1 :: ");
    printf("SUCCESS!\n");

    init_free_bytes = heap_get_free_space();
    init_used_bytes = heap_get_used_space();

    printf("* Test 2: allocating a 400KB chunk :: ");
    if(LOG || TESTING) printf("\n");
    test2();
    if(LOG || TESTING) printf("* Test 2 :: ");
    printf("SUCCESS!\n");

    printf("* Test 3: allocating a chunk that needs extending a space :: ");
    if(LOG || TESTING) printf("\n");
    test3();
    if(LOG || TESTING) printf("* Test 3 :: ");
    printf("SUCCESS!\n");

    printf("* Test 4: resetting a heap :: ");
    if(LOG || TESTING) printf("\n");
    test4();
    if(LOG || TESTING) printf("* Test 4 :: ");
    printf("SUCCESS!\n");

    printf("* Test 5: trying to allocate too big chunk, it shall not pass :: ");
    if(LOG || TESTING) printf("\n");
    test5();
    if(LOG || TESTING) printf("* Test 5 :: ");
    printf("SUCCESS!\n");

    printf("* Test 6: reallocating a chunk - new size is bigger, next chunk is free :: ");
    if(LOG || TESTING) printf("\n");
    test6();
    if(LOG || TESTING) printf("* Test 6 :: ");
    printf("SUCCESS!\n");

    printf("* Test 7: reallocating a chunk - new size is smaller :: ");
    if(LOG || TESTING) printf("\n");
    test7();
    if(LOG || TESTING) printf("* Test 7 :: ");
    printf("SUCCESS!\n");

    printf("* Test 8: reallocating a chunk - new size is bigger, but next chunk is not free :: ");
    if(LOG || TESTING) printf("\n");
    test8();
    if(LOG || TESTING) printf("* Test 8 :: ");
    printf("SUCCESS!\n");

    printf("* Test 9: testing realloc with memblock being NULL, size being 0 (free) or size being unchanged :: ");
    if(LOG || TESTING) printf("\n");
    test9();
    if(LOG || TESTING) printf("* Test 9 :: ");
    printf("SUCCESS!\n");

    printf("* Test 10: testing calloc :: ");
    if(LOG || TESTING) printf("\n");
    test10();
    if(LOG || TESTING) printf("* Test 10 :: ");
    printf("SUCCESS!\n");

    printf("* Test 11: malloc_aligned :: ");
    if(LOG || TESTING) printf("\n");
    test11();
    if(LOG || TESTING) printf("* Test 11 :: ");
    printf("SUCCESS!\n");

    printf("* Test 12: malloc_aligned (sizeof(struct chunk_t) is available on the first page) :: ");
    if(LOG || TESTING) printf("\n");
    test12();
    if(LOG || TESTING) printf("* Test 12 :: ");
    printf("SUCCESS!\n");

    printf("* Test 13: malloc_aligned (there is size equal to wanted, no-split variant) :: ");
    if(LOG || TESTING) printf("\n");
    test13();
    if(LOG || TESTING) printf("* Test 13 :: ");
    printf("SUCCESS!\n");

    printf("* Test 14: malloc_aligned (there's no space and I'm sad) :: ");
    if(LOG || TESTING) printf("\n");
    test14();
    if(LOG || TESTING) printf("* Test 14 :: ");
    printf("SUCCESS!\n");


    printf("* Test n: description :: ");
    if(LOG || TESTING) printf("\n");
    if(LOG || TESTING) printf("* Test n :: ");
    printf("SUCCESS!\n");

    heap_delete(0);
    assert(get_heap()->is_set==0);
    return 0;
}