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

void test15() {
    char *p1 = heap_malloc(100);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    assert(heap_validate()==no_errors);

    struct chunk_t *c;
    c = get_heap()->head_chunk;
    assert(get_pointer_type(c)==pointer_control_block);
    char *p2 = (char*)(c->next)+sizeof(struct chunk_t);
    assert(get_pointer_type(p2)==pointer_unallocated);
    p2 = p1+1;
    assert(get_pointer_type(p2)==pointer_inside_data_block);
    p2 = (char*)(get_heap()->end_fence_p);
    assert(get_pointer_type(p2)==pointer_end_fence);
    p2 = get_heap()->data-1;
    assert(get_pointer_type(p2)==pointer_out_of_heap);
    p2 = (char*)(get_heap()->end_fence_p)+sizeof(int);
    assert(get_pointer_type(p2)==pointer_out_of_heap);
    assert(get_pointer_type(NULL)==pointer_null);

    heap_free(p1);
    assert(heap_validate()==no_errors);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()+heap_get_used_space()==get_heap()->pages*PAGE_SIZE);
}

void test16() {
    char *p1 = heap_malloc(600);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    char *p2 = heap_malloc(400);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    char *p3 = heap_malloc(700);
    assert(p3!=NULL);
    assert(get_pointer_type(p3)==pointer_valid);
    assert(heap_validate()==no_errors);
    heap_free(p2);
    char *p4 = heap_malloc(1750);
    assert(p4!=NULL);
    assert(get_pointer_type(p4)==pointer_valid);
    assert(heap_validate()==no_errors);

    assert(heap_get_free_gaps_count()==2);
    assert(heap_get_used_blocks_count()==3);
    size_t b5_size = heap_get_control_block(p4)->next->size;
    size_t largest=400;
    if(b5_size>400) largest=b5_size;
    heap_dump_debug_information();
    assert(heap_get_largest_free_area()==largest);
    assert(heap_get_largest_used_block_size()==1750);
    assert(heap_get_free_space()==b5_size+400);
    assert(heap_get_used_space()==5*sizeof(struct chunk_t)+600+700+1750+sizeof(int));

    char *p5 = heap_get_data_block_start(p1+10);
    assert(p5==p1);

    heap_free(p1);
    heap_free(p3);
    heap_free(p4);

    assert(heap_validate()==no_errors);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_space()==(get_heap()->pages*PAGE_SIZE-sizeof(struct chunk_t)-sizeof(int)));
    assert(heap_get_largest_free_area()==heap_get_free_space());
    assert(heap_get_used_space()==sizeof(struct chunk_t)+sizeof(int));
    assert(heap_get_largest_used_block_size()==0);
}

void test17() {
    struct chunk_t *chunk = get_heap()->head_chunk;
    size_t size = chunk->size;
    int checksum = chunk->checksum;
    memset(chunk,0,sizeof(struct chunk_t));
    update_chunk_checksum(chunk);
    assert(chunk->checksum==1);
    assert(verify_chunk_checksum(chunk)==0); // checksum is calculated correctly
    assert(heap_validate()!=no_errors);
    
    chunk->first_fence=FIRFENCE;
    chunk->prev=NULL;
    chunk->size=size;
    chunk->next=NULL;
    chunk->debug_line=0;
    chunk->debug_file=NULL;
    chunk->alloc=0;
    chunk->second_fence=SECFENCE;

    update_chunk_checksum(chunk);
    assert(chunk->checksum==checksum);
    assert(verify_chunk_checksum(chunk)==0);
    assert(heap_validate()==no_errors);

    chunk->first_fence=FIRFENCE+1;
    update_chunk_checksum(chunk);
    assert(chunk->checksum!=checksum);
    assert(verify_chunk_checksum(chunk)==0); // checksum is valid, it's the first fence that is invalid.
    assert(heap_validate()==err_chunk_fence1);

    chunk->first_fence=FIRFENCE;
    update_chunk_checksum(chunk);

    checksum=get_heap()->checksum;
    get_heap()->checksum=checksum+5;
    assert(verify_heap_checksum()!=0);
    assert(heap_validate()==err_heap_checksum);
    update_heap_checksum();
    assert(checksum==get_heap()->checksum);
    assert(verify_heap_checksum()==0);
    assert(heap_validate()==no_errors);
}

void test18() {
    assert(heap_validate()==no_errors);
    get_heap()->checksum=get_heap()->checksum+10;
    assert(heap_validate()==err_heap_checksum);
    get_heap()->checksum=get_heap()->checksum-10;
    assert(heap_validate()==no_errors);
    int good_checksum = get_heap()->checksum;

    struct chunk_t *c = get_heap()->head_chunk;
    get_heap()->head_chunk=NULL;
    update_heap_checksum(); //heap checksum will be updated for "bad data" to avoid getting checksum error
    assert(heap_validate()==err_head_is_null);
    get_heap()->head_chunk=c;
    get_heap()->tail_chunk=NULL;
    assert(heap_validate()==err_tail_is_null);
    update_heap_data();
    assert(heap_validate()==no_errors);

    get_heap()->head_chunk=c+1;
    update_heap_checksum();
    assert(heap_validate()==err_invalid_head);
    get_heap()->head_chunk=c;
    update_heap_checksum();
    assert(heap_validate()==no_errors);

    *(get_heap()->end_fence_p)=1;
    assert(heap_validate()==err_end_fence);
    *(get_heap()->end_fence_p)=LASFENCE;
    assert(heap_validate()==no_errors);

    c->first_fence=1;
    assert(heap_validate()==err_chunk_fence1);
    c->first_fence=FIRFENCE;
    c->second_fence=1;
    assert(heap_validate()==err_chunk_fence2);
    c->second_fence=SECFENCE;
    assert(heap_validate()==no_errors);

    c->checksum=c->checksum+10;
    assert(heap_validate()==err_chunk_checksum);
    c->checksum=c->checksum-10;
    assert(heap_validate()==no_errors);

    c->next=c+1;
    update_chunk_checksum(c);
    assert(heap_validate()==err_invalid_next);
    c->next=NULL;
    update_chunk_checksum(c);
    assert(heap_validate()==no_errors);

    c->prev=c-1;
    update_chunk_checksum(c);
    assert(heap_validate()==err_invalid_prev);
    c->prev=NULL;
    update_chunk_checksum(c);
    assert(heap_validate()==no_errors);

    get_heap()->tail_chunk=c+1;
    update_heap_checksum();
    assert(heap_validate()==err_invalid_tail);
    get_heap()->tail_chunk=c;
    update_heap_checksum();
    assert(heap_validate()==no_errors);

    char *p1 = heap_malloc(60);
    assert(p1!=NULL);
    assert(get_pointer_type(p1)==pointer_valid);
    char *p2 = heap_malloc(40);
    assert(p2!=NULL);
    assert(get_pointer_type(p2)==pointer_valid);
    assert(heap_validate()==no_errors);

    struct chunk_t *c1 = heap_get_control_block(p1);
    struct chunk_t *c2 = heap_get_control_block(p2);

    get_heap()->tail_chunk=c1;
    update_heap_checksum();
    assert(heap_validate()==err_invalid_tail);
    get_heap()->tail_chunk=c2;
    update_heap_checksum();
    assert(heap_validate()==err_invalid_tail);
    get_heap()->tail_chunk=c2->next;
    update_heap_checksum();
    assert(heap_validate()==no_errors);

    c2->prev=c2->next;
    update_chunk_checksum(c2);
    assert(heap_validate()==err_invalid_prev);
    c2->prev=c1;
    c1->next=c2->next;
    update_chunk_checksum(c1);
    assert(heap_validate()==err_invalid_next);
    c1->next=c2;
    update_chunk_checksum(c1);
    update_chunk_checksum(c2);
    assert(heap_validate()==no_errors);

    heap_free(p1);
    heap_free(p2);
    assert(heap_validate()==no_errors);
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

    printf("* Test 15: get_pointer_type :: ");
    if(LOG || TESTING) printf("\n");
    test15();
    if(LOG || TESTING) printf("* Test 15 :: ");
    printf("SUCCESS!\n");

    printf("* Test 16: statistics functions :: ");
    if(LOG || TESTING) printf("\n");
    test16();
    if(LOG || TESTING) printf("* Test 16 :: ");
    printf("SUCCESS!\n");

    printf("* Test 17: checksum functions :: ");
    if(LOG || TESTING) printf("\n");
    test17();
    if(LOG || TESTING) printf("* Test 17 :: ");
    printf("SUCCESS!\n");

    printf("* Test 18: validation functions :: ");
    if(LOG || TESTING) printf("\n");
    test18();
    if(LOG || TESTING) printf("* Test 18 :: ");
    printf("SUCCESS!\n");

    heap_delete(0);
    assert(get_heap()->is_set==0);
    return 0;
}