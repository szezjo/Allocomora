#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocomora.h"
#include "custom_unistd.h"

// Control number: 104
static struct heap_t heap;

int heap_setup() {
    if(heap.is_set) {
        printf("Heap is already set up.\n");
        return 0;
    }
    
    heap.data=custom_sbrk(PAGES_BGN*PAGE_SIZE);
    if (heap.data == (void*)-1) return -1;

    struct chunk_t mainchunk;
    memset(&mainchunk,0,sizeof(mainchunk));
    mainchunk.prev=NULL;
    mainchunk.next=NULL;
    mainchunk.size=(PAGES_BGN*PAGE_SIZE)-(sizeof(struct chunk_t)+sizeof(int));
    mainchunk.alloc=0;
    mainchunk.first_fence=FIRFENCE;
    mainchunk.second_fence=SECFENCE;

    memcpy(heap.data,&mainchunk,sizeof(struct chunk_t));
    heap.head_chunk=(struct chunk_t *)heap.data;
    heap.tail_chunk=(struct chunk_t *)heap.data;
    int last_fence=LASFENCE;
    

    heap.is_set=1;
    heap.pages=PAGES_BGN;
    heap.chunks=1;

    update_end_fence();

    return 0;
}

void update_end_fence() {
    int end_fence=LASFENCE;
    int *end_fence_e=(int*)(heap.tail_chunk+sizeof(struct chunk_t)+heap.tail_chunk->size);
    memcpy(end_fence_e,&end_fence,sizeof(int));
    heap.end_fence_p=end_fence_e;
}

void *find_free_chunk(size_t size) {
    struct chunk_t *chunk_to_check=heap.head_chunk;
    size_t best_fit_size=-1;
    struct chunk_t *best_fit=NULL;
    while(chunk_to_check!=NULL) {
        if(chunk_to_check->alloc==0) {
            if((chunk_to_check->size)>(size+sizeof(struct chunk_t))) {
                if(best_fit_size==-1 || best_fit_size>(chunk_to_check->size)) {
                    best_fit_size=chunk_to_check->size;
                    best_fit=chunk_to_check;
                }
            }
            else if(chunk_to_check->size==size) {
                printf("Found chunk with size %lu\n", size);
                return chunk_to_check;
            }
        }
        chunk_to_check=chunk_to_check->next;
    }
    return best_fit;
}

void update_heap_data() {
    struct chunk_t *ch = heap.head_chunk;
    while(ch->next!=NULL) ch=ch->next;
    heap.tail_chunk=ch;
}

void *heap_malloc_debug(size_t count, int fileline, const char* filename) {
    struct chunk_t *chunk_to_alloc = find_free_chunk(count);
    printf("Found a free chunk %p.\n", chunk_to_alloc);
    if(chunk_to_alloc!=NULL) {
        if(chunk_to_alloc->size==count) {
            printf("Using a chunk %p.\n", chunk_to_alloc);
            chunk_to_alloc->alloc=1;
            chunk_to_alloc->debug_line=fileline;
            chunk_to_alloc->debug_file=filename;
            update_heap_data();
            return (void*)((char*)chunk_to_alloc+sizeof(struct chunk_t));
        }
        else if(chunk_to_alloc->size>count+sizeof(struct chunk_t)) {
            printf("Spliiting a chunk %p.\n", chunk_to_alloc);
            struct chunk_t *res=NULL;
            res=split(chunk_to_alloc,count);
            if (res==NULL) {
                printf("Can't split a chunk.\n");
                return NULL;
            }
            res->alloc=1;
            res->debug_line=fileline;
            res->debug_file=filename;
            update_heap_data();
            return (void*)((char*)res+sizeof(struct chunk_t));
        }
    }
    // free block not found. asking for more pages
    size_t wanted_size;
    if (heap.tail_chunk->alloc) wanted_size=count+sizeof(struct chunk_t);
    else wanted_size=count;
    intptr_t wanted_memory = PAGE_SIZE*((wanted_size/PAGE_SIZE)+(!!(wanted_size%PAGE_SIZE)));
    if (custom_sbrk(wanted_memory)==(void*)-1) return NULL;
    heap.pages+=wanted_memory/PAGE_SIZE;

    if(heap.tail_chunk->alloc) {
        struct chunk_t new_chunk;
        struct chunk_t *new_tail = heap.tail_chunk+heap.tail_chunk->size+sizeof(struct chunk_t);

        new_chunk.first_fence=FIRFENCE;
        new_chunk.second_fence=SECFENCE;
        new_chunk.size=wanted_memory;
        new_chunk.prev=heap.tail_chunk;
        new_chunk.next=NULL;
        heap.tail_chunk->next=new_tail;
        new_chunk.alloc=0;
        
        memcpy(new_tail,&new_chunk,sizeof(struct chunk_t));
        heap.tail_chunk=new_tail;
        heap.chunks++;
    }
    else {
        heap.tail_chunk->size=heap.tail_chunk->size+wanted_memory;
    }

    update_end_fence();
    return heap_malloc_debug(count,fileline,filename); //try allocating again, now with more space.
}

struct chunk_t *merge(struct chunk_t *chunk1, struct chunk_t *chunk2) {
    if(chunk1==NULL || chunk2==NULL) return NULL;
    if(chunk2->next==chunk1) return merge(chunk2, chunk1);
    if(chunk1->next!=chunk2) return NULL;
    if(chunk1->alloc==1 || chunk2->alloc==1) return NULL;
    printf("Merging %p with %p",chunk1,chunk2);

    chunk1->size=chunk1->size+chunk2->size+sizeof(struct chunk_t);
    chunk1->next=chunk2->next;
    if(chunk1->next) chunk1->next->prev=chunk1;
    update_heap_data();
    return chunk1;
}
struct chunk_t *split(struct chunk_t *chunk_to_split, size_t size) {
    struct chunk_t cut;
    cut.size=chunk_to_split->size-size-sizeof(struct chunk_t);
    cut.first_fence=FIRFENCE;
    cut.second_fence=SECFENCE;
    cut.alloc=0;
    cut.prev=chunk_to_split;
    cut.next=chunk_to_split->next;
    
    struct chunk_t *cut_p = chunk_to_split+sizeof(struct chunk_t)+size;
    memcpy(cut_p,&cut,sizeof(struct chunk_t));
    chunk_to_split->size=size;
    chunk_to_split->next=cut_p;
    heap.chunks++;

    return chunk_to_split;
}

int main() {
    int tmp = heap_setup();
    //struct chunk_t *p = find_free_chunk(150);
    printf("err: %d\n",tmp);
    printf("End fence address: %p\n",heap.end_fence_p);
    printf("something\n");
    struct chunk_t *p = find_free_chunk(1);
    if(p==NULL) printf("returned NULL\n");
    else printf("size of found block: %lu\n",p->size);
    //printf("fence 1: %d\n",p->first_fence);
    //find_free_chunk(150);
    struct chunk_t *chk = heap_malloc_debug(4024, __LINE__, __FILE__);
    printf("Size of chunk 1: %lu\n",heap.head_chunk->size);
    //printf("Size of chunk 2: %lu\n",heap.head_chunk->next->size);
    //printf("Size of chunk 3: %lu\n",heap.head_chunk->next->next->size);
    printf("Size of chunk_t: %lu\n",sizeof(struct chunk_t));
    printf("Updated end fence\n");
    printf("Fence at the end: %d\n",*(heap.end_fence_p));
    printf("End fence address: %p\n",heap.end_fence_p);
    printf("Size of tail chunk: %lu\n",heap.tail_chunk->size);
}