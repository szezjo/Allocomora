#include "allocomora.h"
#include "custom_unistd.h"

int main() {
    int int_status;
    enum validation_type_t val_status;
    
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
    assert(heap_get_used_blocks_count()==0);
    assert(heap_get_free_gaps_count()==1);
    assert(heap_get_free_space()==init_free_bytes);
    assert(heap_get_used_space()==init_used_bytes);
    printf("* Test 2: success!\n");

    heap_delete(0);
    return 0;
}