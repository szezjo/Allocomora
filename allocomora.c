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

void heap_free(void* memblock) {
    if(get_pointer_type(memblock)!=pointer_valid) return;
    struct chunk_t *chunk = (struct chunk_t *)((char*)memblock-sizeof(struct chunk_t));
    chunk->alloc=0;

    if(chunk->prev!=NULL && chunk->prev->alloc==0) chunk=merge(chunk->prev,chunk,1);
    if(chunk->next!=NULL && chunk->next->alloc==0) chunk=merge(chunk,chunk->next,1);

    update_heap_data();
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

enum pointer_type_t get_pointer_type(const void* pointer) {
    if(pointer==NULL) return pointer_null;
    const char *p = (char*)pointer; // to perform pointer arithmetic
    if(p<(char*)heap.data || p>=(char*)(heap.tail_chunk+heap.tail_chunk->size+sizeof(struct chunk_t)+sizeof(int))) return pointer_out_of_heap;
    if(p>=(char*)heap.end_fence_p && p<(char*)heap.end_fence_p+sizeof(int)) return pointer_end_fence;
    struct chunk_t *i=heap.head_chunk;
    while(i!=NULL) {
        if(p>=(char*)i && p<(char*)i+sizeof(struct chunk_t)+i->size) {
            if(p>=(char*)i && p<(char*)i+sizeof(struct chunk_t)) return pointer_control_block;
            if(i->alloc==0) return pointer_unallocated;
            if(p==(char*)i+sizeof(struct chunk_t)) return pointer_valid;
            return pointer_inside_data_block;
        }
        i=i->next;
    }
    return pointer_out_of_heap;
}

void print_pointer_type(const void* pointer) {
    enum pointer_type_t type = get_pointer_type(pointer);
    if(type==pointer_null) printf("[%p] is null\n", pointer);
    else if (type==pointer_out_of_heap) printf("[%p] is out of heap\n", pointer);
    else if (type==pointer_control_block) printf("[%p] is inside control block\n", pointer);
    else if (type==pointer_inside_data_block) printf("[%p] is inside data block\n", pointer);
    else if (type==pointer_unallocated) printf("[%p] is unallocated\n", pointer);
    else if (type==pointer_valid) printf("[%p] is valid\n",pointer);
    else if (type==pointer_end_fence) printf("[%p] is at the end fence\n", pointer);
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

void *heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename) {
    size_t size_to_alloc = number*size;
    struct chunk_t *p = heap_malloc_debug(size_to_alloc,fileline,filename);
    if(p==NULL) return NULL;
    memset(p,0,size_to_alloc);
    return (void*)((char*)p+sizeof(struct chunk_t));
}

void *heap_realloc_debug(void *memblock, size_t size, int fileline, const char *filename) {
    if(memblock==NULL) return heap_malloc_debug(size,fileline,filename);
    if(size==0) {
        heap_free(memblock);
        return NULL;
    }
    struct chunk_t *chunk = (struct chunk_t *)((char*)memblock-sizeof(struct chunk_t));
    if(chunk->size==size) return memblock;
    if(chunk->size>size+sizeof(struct chunk_t)) {
        split(chunk,size);
        return memblock;
    }
    if(chunk->next && chunk->next->alloc==0 && chunk->next->size+chunk->size+sizeof(struct chunk_t)>size) {
        merge(chunk,chunk->next,0);
        split(chunk,size);
        return memblock;
    }

    char *p = heap_malloc_debug(size, fileline, filename);
    if (p==NULL) return NULL;
    memcpy(p,memblock,chunk->size);
    heap_free(memblock);
    return p;
}

void *heap_malloc(size_t count) {
    return heap_malloc_debug(count,0,NULL);
}
void *heap_calloc(size_t number, size_t size) {
    return heap_calloc_debug(number,size,0,NULL);
}
void *heap_realloc(void *memblock, size_t size) {
    return heap_realloc_debug(memblock,size,0,NULL);
}
struct chunk_t *merge(struct chunk_t *chunk1, struct chunk_t *chunk2, char safe_mode) {
    if(chunk1==NULL || chunk2==NULL) return NULL;
    if(chunk2->next==chunk1) return merge(chunk2, chunk1, safe_mode);
    if(chunk1->next!=chunk2) return NULL;
    if(safe_mode==1 && chunk1->alloc==1 || chunk2->alloc==1) return NULL;
    printf("Merging %p (%ld) with %p (%ld)\n",chunk1,chunk1->size,chunk2,chunk2->size);

    chunk1->size=chunk1->size+chunk2->size+sizeof(struct chunk_t);
    chunk1->next=chunk2->next;
    if(chunk1->next) chunk1->next->prev=chunk1;
    update_heap_data();
    heap.chunks--;
    printf("Merged %p (%ld)\n",chunk1,chunk1->size);
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
    if(cut_p->next && cut_p->next->alloc==0) merge(cut_p,cut_p->next,1);
    return chunk_to_split;
}

int main() {
    int tmp = heap_setup();
    //struct chunk_t *p = find_free_chunk(150);
    /* printf("err: %d\n",tmp);
    printf("End fence address: %p\n",heap.end_fence_p);
    printf("something\n");
    struct chunk_t *p = find_free_chunk(1);
    if(p==NULL) printf("returned NULL\n");
    else printf("size of found block: %lu\n",p->size);
    //printf("fence 1: %d\n",p->first_fence);
    //find_free_chunk(150);
    struct chunk_t *chk = heap_malloc_debug(3000, __LINE__, __FILE__);
    printf("Size of chunk 1: %lu\n",heap.head_chunk->size);
    //printf("Size of chunk 2: %lu\n",heap.head_chunk->next->size);
    //printf("Size of chunk 3: %lu\n",heap.head_chunk->next->next->size);
    printf("Size of chunk_t: %lu\n",sizeof(struct chunk_t));
    printf("Updated end fence\n");
    printf("Fence at the end: %d\n",*(heap.end_fence_p));
    printf("End fence address: %p\n",heap.end_fence_p);
    printf("Size of tail chunk: %lu\n",heap.tail_chunk->size);

    printf("Out of heap address(?) %p\n",heap.tail_chunk+heap.tail_chunk->size+sizeof(struct chunk_t)+sizeof(int));
    print_pointer_type(NULL);
    print_pointer_type(heap.end_fence_p);
    print_pointer_type(heap.tail_chunk+heap.tail_chunk->size+sizeof(struct chunk_t));
    print_pointer_type(heap.tail_chunk+heap.tail_chunk->size+sizeof(struct chunk_t)+sizeof(int));
    print_pointer_type(heap.head_chunk);
    print_pointer_type(chk);
    print_pointer_type(chk+4);
    print_pointer_type(find_free_chunk(1)+sizeof(struct chunk_t)); */

    //split(heap.head_chunk,2000);
    //printf("%ld %ld\n", heap.head_chunk->size, heap.head_chunk->next->size );
    //merge(heap.head_chunk,heap.head_chunk->next);

    printf("Chunks: %d\n",heap.chunks);
    char *str = heap_malloc(50);
    strcpy(str,"Ala ma kota");
    printf("%s\n",str);
    printf("Chunks: %d\n",heap.chunks);
    char *str2 = heap_realloc(str,12);
    printf("%s\n%s\n",str,str2);
    printf("Chunks: %d\n",heap.chunks);
    int dif = (heap.head_chunk->next - heap.head_chunk)-sizeof(struct chunk_t);
    printf("%d\n",dif);
    
    heap_free(str2);
    printf("Chunks: %d\n",heap.chunks);
    
}