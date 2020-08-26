#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include "allocomora.h"
#include "custom_unistd.h"

// Control number: 110

// Static variables
static struct heap_t heap;
static pthread_mutex_t heap_mtx;
static pthread_mutexattr_t heap_mtxa;

// Heap basic functions
int heap_setup() {
    if(heap.is_set) {
        printf("-Log- Heap is already set up.\n");
        return 0;
    }
    
    heap.data=custom_sbrk(PAGES_BGN*PAGE_SIZE);
    if (heap.data == (void*)-1) {
        if(LOG) printf("-Log- sbrk() error.\n");
        return -1;
    }

    struct chunk_t mainchunk;
    memset(&mainchunk,0,sizeof(mainchunk));
    mainchunk.prev=NULL;
    mainchunk.next=NULL;
    mainchunk.size=(PAGES_BGN*PAGE_SIZE)-(sizeof(struct chunk_t)+sizeof(int));
    mainchunk.alloc=0;
    mainchunk.first_fence=FIRFENCE;
    mainchunk.second_fence=SECFENCE;
    mainchunk.debug_file=NULL;
    mainchunk.debug_line=0;

    memcpy(heap.data,&mainchunk,sizeof(struct chunk_t));
    heap.head_chunk=(struct chunk_t *)heap.data;
    heap.tail_chunk=(struct chunk_t *)heap.data;
    int last_fence=LASFENCE;
    
    pthread_mutexattr_init(&heap_mtxa);
    pthread_mutexattr_settype(&heap_mtxa,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&heap_mtx,&heap_mtxa);

    heap.is_set=1;
    heap.pages=PAGES_BGN;
    heap.chunks=1;

    update_end_fence();
    update_chunk_checksum(heap.head_chunk);
    update_heap_checksum();
    if(LOG) printf("-Log- Heap successfully initialized.\n");

    return 0;
}

int heap_delete(int force_mode) {
    // If "force mode" is 1, all allocated blocks will be freed automatically.
    if(!heap.is_set) {
        if(LOG) printf("-Log- Heap isn't initialized.\n");
        return 1;
    }
    if(heap_validate()!=no_errors) {
        if(LOG) printf("-Log- Heap is corrupted.\n");
        return 2;
    }
    
    pthread_mutex_destroy(&heap_mtx);
    pthread_mutexattr_destroy(&heap_mtxa);

    struct chunk_t *p = heap.head_chunk;
    while(p) {
        if(p->alloc) {
            if(force_mode==1) heap_free(p);
            else {
                if(LOG) printf("-Log- Some blocks are still allocated. Use \"force mode\" to free them automatically or heap_free() to free them manually.\n");
                return 3;
            }
        }
        p=p->next;
    }
    void *check=custom_sbrk(-(heap_get_used_space()+heap_get_free_space()));
    if(check==(void*)-1) {
        if(LOG) printf("-Log- sbrk() error.\n");
        return -1;
    }
    heap.is_set=0;
    if(LOG) printf("-Log- Heap successfully deleted.\n");
    return 0;
}

int heap_reset(int force_mode) {
    if(LOG) printf("-Log- Resetting a heap.\n");
    int res = heap_delete(force_mode);
    if(res) return res;
    return heap_setup();
}

// *alloc functions
void *heap_malloc_debug(size_t count, int fileline, const char* filename) {
    pthread_mutex_lock(&heap_mtx);
    struct chunk_t *chunk_to_alloc = find_free_chunk(count);
    if(chunk_to_alloc!=NULL) {
        if(LOG) printf("-Log- Found a free chunk %p (%lu).\n", chunk_to_alloc, chunk_to_alloc->size);
        if(chunk_to_alloc->size==count) {
            chunk_to_alloc->alloc=1;
            chunk_to_alloc->debug_line=fileline;
            chunk_to_alloc->debug_file=filename;
            update_heap_data();
            update_chunk_checksum(chunk_to_alloc);
            pthread_mutex_unlock(&heap_mtx);
            return (void*)((char*)chunk_to_alloc+sizeof(struct chunk_t));
        }
        else if(chunk_to_alloc->size>count+sizeof(struct chunk_t)) {
            if(LOG) printf("-Log- Chunk is too large. Splitting.\n");
            struct chunk_t *res=NULL;
            res=split(chunk_to_alloc,count);
            if (res==NULL) {
                if(LOG) printf("-Log- Can't split a chunk.\n");
                pthread_mutex_unlock(&heap_mtx);
                return NULL;
            }
            res->alloc=1;
            res->debug_line=fileline;
            res->debug_file=filename;
            update_chunk_checksum(res);
            update_heap_data();
            pthread_mutex_unlock(&heap_mtx);
            return (void*)((char*)res+sizeof(struct chunk_t));
        }
    }
    if(LOG) printf("-Log- Free block not found. Asking for more space.\n");
    size_t wanted_size;
    if (heap.tail_chunk->alloc) wanted_size=count+sizeof(struct chunk_t);
    else wanted_size=count;
    intptr_t wanted_memory = PAGE_SIZE*((wanted_size/PAGE_SIZE)+(!!(wanted_size%PAGE_SIZE)));
    if (custom_sbrk(wanted_memory)==(void*)-1) {
        pthread_mutex_unlock(&heap_mtx);
        if(LOG) printf("-Log- sbrk() error.\n");
        return NULL;
    }
    heap.pages+=wanted_memory/PAGE_SIZE;
    if(LOG) printf("-Log- Pages increased to %d.\n",heap.pages);

    if(heap.tail_chunk->alloc) {
        if(LOG) printf("-Log- Tail chunk is allocated. Creating a new chunk\n");
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
        update_chunk_checksum(new_tail);
    }
    else {
        if(LOG) printf("-Log- Tail chunk is free. Extending it.\n");
        heap.tail_chunk->size=heap.tail_chunk->size+wanted_memory;
    }

    if(TESTING) printf("-Testing- #1\n");
    update_chunk_checksum(heap.tail_chunk);
    if(TESTING) printf("-Testing- #2\n");
    update_end_fence();
    if(TESTING) printf("-Testing- #3\n");
    if(LOG) printf("-Log- Heap size successfully increased.\n");
    pthread_mutex_unlock(&heap_mtx);
    return heap_malloc_debug(count,fileline,filename); //try allocating again, now with more space.
}

void *heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename) {
    size_t size_to_alloc = number*size;
    struct chunk_t *p = heap_malloc_debug(size_to_alloc,fileline,filename);
    if(p==NULL) return NULL;
    memset(p,0,size_to_alloc);
    return p;
}

void *heap_realloc_debug(void *memblock, size_t size, int fileline, const char *filename) {
    if(memblock==NULL) return heap_malloc_debug(size,fileline,filename);
    if(size==0) {
        if(LOG) printf("-Log- Called realloc with size 0. A chunk will be freed.\n");
        heap_free(memblock);
        return NULL;
    }
    struct chunk_t *chunk = (struct chunk_t *)((char*)memblock-sizeof(struct chunk_t));
    if(chunk->size==size) return memblock;
    
    pthread_mutex_lock(&heap_mtx);
    if(chunk->size>size+sizeof(struct chunk_t)) {
        if(LOG) printf("-Log- Called realloc with smaller size than chunk's size. Splitting.\n");
        split(chunk,size);
        return memblock;
    }
    if(chunk->next && chunk->next->alloc==0 && chunk->next->size+chunk->size+sizeof(struct chunk_t)>size) {
        if(LOG) printf("-Log- Found a free chunk next to given memblock. Merging and splitting.\n");
        merge(chunk,chunk->next,0);
        split(chunk,size);
        return memblock;
    }

    if(LOG) printf("-Log- Using malloc-copy-free method.\n");
    pthread_mutex_unlock(&heap_mtx);

    char *p = heap_malloc_debug(size, fileline, filename);
    if (p==NULL) return NULL;
    pthread_mutex_lock(&heap_mtx);
    memcpy(p,memblock,chunk->size);
    pthread_mutex_unlock(&heap_mtx);
    heap_free(memblock);
    return p;
}

// *alloc_aligned functions
void *heap_malloc_aligned_debug(size_t count, int fileline, const char *filename) {
    if(heap.pages<2) {
        if(LOG) printf("-Log- Aligned malloc requires min. 2 chunks.\n");
        return NULL;
    }
    pthread_mutex_lock(&heap_mtx);
    struct chunk_t *p = heap.head_chunk;
    while(p) {
        if(p->alloc==0) {
            size_t dist = calc_dist(p);
            if(!is_aligned(dist)) {
                if(p->size==count) {
                    if(LOG) printf("-Log- Found a needed chunk. Allocating.\n");
                    p->alloc=1;
                    p->debug_line=fileline;
                    p->debug_file=filename;
                    update_chunk_checksum(p);
                    update_heap_data();
                    pthread_mutex_unlock(&heap_mtx);
                    return (void*)((char*)p+sizeof(struct chunk_t));
                }
                else if (p->size>count) {
                    if(LOG) printf("-Log- Found a needed chunk, but with more size. Splitting and allocating.\n");
                    struct chunk_t *res=NULL;
                    res=split(p,count);
                    if(res==NULL) {
                        printf("Can't split a chunk.\n");
                        pthread_mutex_unlock(&heap_mtx);
                        return NULL;
                    }
                    res->alloc=1;
                    res->debug_line=fileline;
                    res->debug_file=filename;
                    update_chunk_checksum(res);
                    update_heap_data();
                    pthread_mutex_unlock(&heap_mtx);
                    return (void*)((char*)res+sizeof(struct chunk_t));
                }
            }
            if(p->size>=count+2*sizeof(struct chunk_t)) {
                size_t size_in_page = calc_size_in_page(p, dist);
                if(p->size>=size_in_page) {
                    size_t remainder = p->size-size_in_page;
                    if(remainder>=count+sizeof(struct chunk_t) && size_in_page>=sizeof(struct chunk_t)) {
                        if(LOG) printf("-Log- Found a needed chunk, but with more size. Splitting and allocating.\n");
                        struct chunk_t *res=NULL;
                        res=split(p,size_in_page-sizeof(struct chunk_t));
                        if(res==NULL) {
                            if(LOG) printf("-Log- Can't split a chunk.\n");
                            pthread_mutex_unlock(&heap_mtx);
                            return NULL;
                        }
                        res=split(res->next,count);
                        if(res==NULL) {
                            if(LOG) printf("-Log- Can't split a second chunk.\n");
                            merge(p,p->next,1);
                            pthread_mutex_unlock(&heap_mtx);
                            return NULL;
                        }
                        res->alloc=1;
                        res->debug_line=fileline;
                        res->debug_file=filename;
                        update_chunk_checksum(res);
                        update_heap_data();
                        pthread_mutex_unlock(&heap_mtx);
                        return (void*)((char*)res+sizeof(struct chunk_t));
                    }
                    else if (remainder==count && size_in_page>=sizeof(struct chunk_t)) {
                        if(LOG) printf("-Log- Found a needed chunk, but with more size. Splitting and allocating.\n");
                        struct chunk_t *res=NULL;
                        res=split(p,size_in_page-sizeof(struct chunk_t));
                        if(res==NULL) {
                            if(LOG) printf("-Log- Can't split a chunk.\n");
                            pthread_mutex_unlock(&heap_mtx);
                            return NULL;
                        }
                        if(res->next->size!=count) {
                            if(LOG) printf("-Log- Something went wrong with splitting. Please validate a heap for more details.\n");
                            merge(res,res->next,1);
                            pthread_mutex_unlock(&heap_mtx);
                            return NULL;
                        }
                        res->next->alloc=1;
                        res->next->debug_line=fileline;
                        res->next->debug_file=filename;
                        update_chunk_checksum(res->next);
                        update_heap_data();
                        pthread_mutex_unlock(&heap_mtx);
                        return (void*)((char*)res->next+sizeof(struct chunk_t));
                    }
                }
            }
        }
        p=p->next;
    }
    if(LOG) printf("-Log- A needed chunk couldn't be found.\n");
    pthread_mutex_unlock(&heap_mtx);
    return NULL;
}

void *heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename) {
    size_t size_to_alloc = number*size;
    struct chunk_t *p = heap_malloc_aligned_debug(size_to_alloc,fileline,filename);
    if(p==NULL) return NULL;
    memset(p,0,size_to_alloc);
    return p;
}

void *heap_realloc_aligned_debug(void *memblock, size_t size, int fileline, const char *filename) {
    if(memblock==NULL) return heap_malloc_aligned_debug(size,fileline,filename);
    if(size==0) {
        heap_free(memblock);
        return NULL;
    }
    struct chunk_t *chunk = (struct chunk_t *)((char*)memblock-sizeof(struct chunk_t));
    if(chunk->size==size) return memblock;
    size_t dist = calc_dist(chunk);
    if(!is_aligned(dist) && chunk->size>size+sizeof(struct chunk_t)) {
        if(LOG) printf("-Log- Found a chunk, but with more size. Splitting.\n");
        split(chunk,size);
        return memblock;
    }
    if(!is_aligned(dist) && chunk->next && chunk->next->alloc==0 && chunk->next->size+chunk->size+sizeof(struct chunk_t)>size) {
        if(LOG) printf("-Log- Found a chunk next to given memblock. Merging.\n");
        merge(chunk,chunk->next,0);
        split(chunk,size);
        return memblock;
    }

    if(LOG) printf("-Log- Trying malloc-copy-free method.\n");
    char *p = heap_malloc_aligned_debug(size, fileline, filename);
    if (p==NULL) return NULL;
    pthread_mutex_lock(&heap_mtx);
    memcpy(p,memblock,chunk->size);
    pthread_mutex_unlock(&heap_mtx);
    heap_free(memblock);
    return p;
}

// Non-debug *alloc and *alloc_aligned functions
void *heap_malloc(size_t count) {
    return heap_malloc_debug(count,0,NULL);
}
void *heap_calloc(size_t number, size_t size) {
    return heap_calloc_debug(number,size,0,NULL);
}
void *heap_realloc(void *memblock, size_t size) {
    return heap_realloc_debug(memblock,size,0,NULL);
}
void *heap_malloc_aligned(size_t count) {
    return heap_malloc_aligned_debug(count,0,NULL);
}
void *heap_calloc_aligned(size_t number, size_t size) {
    return heap_calloc_aligned_debug(number,size,0,NULL);
}
void *heap_realloc_aligned(void* memblock, size_t size) {
    return heap_realloc_aligned_debug(memblock,size,0,NULL);
}

// Chunk management functions
void heap_free(void* memblock) {
    pthread_mutex_lock(&heap_mtx);
    if(get_pointer_type(memblock)!=pointer_valid) {
        if(LOG) printf("-Log- A pointer is not valid and can't be used in heap_free().\n");
        pthread_mutex_unlock(&heap_mtx);
        return;
    }
    struct chunk_t *chunk = (struct chunk_t *)((char*)memblock-sizeof(struct chunk_t));
    chunk->alloc=0;

    if(chunk->prev!=NULL && chunk->prev->alloc==0) chunk=merge(chunk->prev,chunk,1);
    if(chunk->next!=NULL && chunk->next->alloc==0) chunk=merge(chunk,chunk->next,1);

    update_chunk_checksum(chunk);
    update_heap_data();
    if(LOG) printf("-Log- A block is successfully freed.\n");
    pthread_mutex_unlock(&heap_mtx);
}

struct chunk_t *merge(struct chunk_t *chunk1, struct chunk_t *chunk2, char safe_mode) {
    if(chunk1==NULL || chunk2==NULL) return NULL;
    if(chunk2->next==chunk1) return merge(chunk2, chunk1, safe_mode);
    if(chunk1->next!=chunk2) return NULL;
    if(safe_mode==1 && chunk1->alloc==1 || chunk2->alloc==1) return NULL;
    if(LOG) printf("-Log- Merging %p (%ld) with %p (%ld)\n",chunk1,chunk1->size,chunk2,chunk2->size);

    chunk1->size=chunk1->size+chunk2->size+sizeof(struct chunk_t);
    chunk1->next=chunk2->next;
    if(chunk1->next) {
        chunk1->next->prev=chunk1;
        update_chunk_checksum(chunk1->next);
    }
    heap.chunks--;
    update_heap_data();
    update_chunk_checksum(chunk1);
    if(LOG) printf("-Log- Merged %p (%ld)\n",chunk1,chunk1->size);
    return chunk1;
}
struct chunk_t *split(struct chunk_t *chunk_to_split, size_t size) {
    if(chunk_to_split->size==size) return chunk_to_split;
    if(chunk_to_split->size<size) {
        if(LOG) printf("-Log- Given size is bigger than chunk's size. Aborting.\n");
        return NULL;
    }
    struct chunk_t cut;
    cut.size=chunk_to_split->size-size-sizeof(struct chunk_t);
    cut.first_fence=FIRFENCE;
    cut.second_fence=SECFENCE;
    cut.alloc=0;
    cut.debug_file=NULL;
    cut.debug_line=0;
    cut.prev=chunk_to_split;
    cut.next=chunk_to_split->next;
    
    struct chunk_t *cut_p = (struct chunk_t *)((char*)chunk_to_split+sizeof(struct chunk_t)+size);
    if(TESTING) printf("-Testing- split(): before memcpy\n");
    memcpy(cut_p,&cut,sizeof(struct chunk_t));
    if(TESTING) printf("-Testing- split(): after memcpy\n");
    chunk_to_split->size=size;
    chunk_to_split->next=cut_p;
    heap.chunks++;
    if(cut_p->next) {
        if (cut_p->next->alloc==0) merge(cut_p,cut_p->next,1);
        else {
            cut_p->next->prev=cut_p;
            update_chunk_checksum(cut_p->next);
        }
    }
    update_chunk_checksum(cut_p);
    update_chunk_checksum(chunk_to_split);
    update_heap_data();
    update_heap_checksum();
    return chunk_to_split;
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
                if(LOG) printf("-Log- Found chunk with size %lu\n", size);
                return chunk_to_check;
            }
        }
        chunk_to_check=chunk_to_check->next;
    }
    if(LOG && best_fit!=NULL) printf("-Log- Found chunk with size %lu\n",best_fit->size);
    return best_fit;
}

// Heap control functions
enum pointer_type_t get_pointer_type(const void* pointer) {
    if(pointer==NULL) return pointer_null;
    char *p = (char*)pointer; // to perform pointer arithmetic
    if(p<(char*)heap.data || p>=((char*)heap.tail_chunk+heap.tail_chunk->size+sizeof(struct chunk_t)+sizeof(int))) return pointer_out_of_heap;
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

void update_heap_data() {
    struct chunk_t *ch = heap.head_chunk;
    while(ch->next!=NULL) ch=ch->next;
    heap.tail_chunk=ch;

    update_heap_checksum();
}

void update_end_fence() {
    pthread_mutex_lock(&heap_mtx);
    int end_fence=LASFENCE;
    int *end_fence_e=(int*)((char*)heap.tail_chunk+sizeof(struct chunk_t)+heap.tail_chunk->size);
    if(TESTING) printf("-Testing- update_end_fence() before memcpy\n");
    memcpy(end_fence_e,&end_fence,sizeof(int));
    heap.end_fence_p=end_fence_e;
    update_heap_checksum();
    pthread_mutex_unlock(&heap_mtx);
}

// Statistics functions
void* heap_get_data_block_start(const void* pointer) {
    enum pointer_type_t type = get_pointer_type(pointer);
    if(type==pointer_valid) return (void*)pointer;
    if(type!=pointer_inside_data_block) return NULL;

    struct chunk_t *tmp = heap.head_chunk;
    while(tmp) {
        if((char*)pointer>=(char*)tmp && (char*)pointer<(char*)tmp+sizeof(struct chunk_t)+tmp->size) return (void*)((char*)tmp+sizeof(struct chunk_t));
        tmp=tmp->next;
    }
    return NULL;
}

size_t heap_get_used_space(void) {
    struct chunk_t *tmp = heap.head_chunk;
    size_t size=0;
    while(tmp) {
        size+=sizeof(struct chunk_t);
        if(tmp->alloc) size+=tmp->size;
        tmp=tmp->next;
    }
    size+=sizeof(int); //size of the end fence
    return size;
}
size_t heap_get_largest_used_block_size(void) {
    struct chunk_t *tmp = heap.head_chunk;
    size_t max=0;
    while(tmp) {
        if(tmp->alloc && tmp->size>max) max=tmp->size;
        tmp=tmp->next;
    }
    return max;
}

uint64_t heap_get_used_blocks_count(void) {
    struct chunk_t *tmp = heap.head_chunk;
    uint64_t count=0;
    while(tmp) {
        if(tmp->alloc) count++;
        tmp=tmp->next;
    }
    return count;
}

size_t heap_get_free_space(void) {
    struct chunk_t *tmp = heap.head_chunk;
    size_t size=0;
    while(tmp) {
        if(tmp->alloc==0) size+=tmp->size;
        tmp=tmp->next;
    }
    return size;
}

size_t heap_get_largest_free_area(void) {
    struct chunk_t *tmp = heap.head_chunk;
    size_t max=0;
    while(tmp) {
        if(tmp->alloc==0 && tmp->size>max) max=tmp->size;
        tmp=tmp->next;
    }
    return max;
}

uint64_t heap_get_free_gaps_count(void) {
    struct chunk_t *tmp = heap.head_chunk;
    uint64_t count=0;
    while(tmp) {
        if(tmp->alloc==0 && tmp->size>=sizeof(void*)+sizeof(struct chunk_t)) count++;
        tmp=tmp->next;
    }
    return count;
}

size_t heap_get_block_size(const void* memblock) {
    if (get_pointer_type(memblock)!=pointer_valid) return 0;
    struct chunk_t *tmp = heap_get_control_block(memblock);
    return tmp->size;
}

// Checksum functions
void update_chunk_checksum(struct chunk_t *chunk) {
    chunk->checksum=1;
    int newsum=0;
    for(int i=0; i<sizeof(struct chunk_t); i++) {
        newsum+=*(((char*)chunk)+i);
    }
    chunk->checksum=newsum;
}

void update_heap_checksum() {
    heap.checksum=1;
    int newsum=0;
    struct heap_t *p = &heap;
    for(int i=0; i<sizeof(struct heap_t); i++) {
        newsum+=*(((char*)p)+i);
    }
    heap.checksum=newsum;
}

int verify_chunk_checksum(struct chunk_t *chunk) {
    int oldsum=chunk->checksum;
    update_chunk_checksum(chunk);
    int newsum=chunk->checksum;
    chunk->checksum=oldsum;
    if(oldsum!=newsum) return 1;
    return 0;
}

int verify_heap_checksum() {
    int oldsum=heap.checksum;
    update_heap_checksum();
    int newsum=heap.checksum;
    heap.checksum=oldsum;
    if(oldsum!=newsum) return 1;
    return 0;
}

// Calculation functions
size_t calc_dist(struct chunk_t *chunk) {
    struct chunk_t *p = heap.head_chunk;
    size_t dist=0;
    while(p!=chunk) {
        dist+=sizeof(struct chunk_t)+p->size;
        if(p->next) p=p->next;
        else return 0;
    }
    dist+=sizeof(struct chunk_t);
    return dist;
}

size_t calc_size_in_page(struct chunk_t *chunk, size_t dist) {
    int page=dist/PAGE_SIZE+1;
    size_t size = PAGE_SIZE-(dist%PAGE_SIZE);
    if(page==heap.pages) size-=4;
    if(chunk->size<size) return chunk->size;
    return size;
}

int is_aligned(size_t dist) {
    return !!(dist%PAGE_SIZE);
}


// Validation functions
enum validation_code_t heap_validate() {
    if(verify_heap_checksum()) return err_heap_checksum;
    if(heap.head_chunk==NULL) return err_head_is_null;
    if(heap.tail_chunk==NULL) return err_tail_is_null;
    if((char*)heap.head_chunk!=(char*)heap.data) return err_invalid_head;
    if(*(heap.end_fence_p)!=LASFENCE) return err_end_fence;

    struct chunk_t *p = heap.head_chunk;
    struct chunk_t *prev = NULL;
    while(p) {
        if(p->first_fence!=FIRFENCE) return err_chunk_fence1;
        if(p->second_fence!=SECFENCE) return err_chunk_fence2;
        if(verify_chunk_checksum(p)) return err_chunk_checksum;
        if(p->next && p->next!=(struct chunk_t *)((char*)p+sizeof(struct chunk_t)+p->size)) return err_invalid_next;
        if(p->prev!=prev) return err_invalid_prev;
        prev=p;
        p=p->next;
    }
    if(heap.tail_chunk!=prev) return err_invalid_tail;
    return no_errors;
}

enum validation_code_t validate_and_print() {
    enum validation_code_t ret = heap_validate();
    if(ret==0) printf("[Heap validation] No errors\n");
    else if(ret==1) printf("[Heap validation] Heap checksum error\n");
    else if(ret==2) printf("[Heap validation] Head is NULL\n");
    else if(ret==3) printf("[Heap validation] Tail is NULL\n");
    else if(ret==4) printf("[Heap validation] End fence error\n");
    else if(ret==5) printf("[Heap validation] First fence error\n");
    else if(ret==6) printf("[Heap validation] Second fence error\n");
    else if(ret==7) printf("[Heap validation] Chunk checksum error\n");
    else if(ret==8) printf("[Heap validation] Invalid prev\n");
    else if(ret==9) printf("[Heap validation] Invalid next\n");
    else if(ret==10) printf("[Heap validation] Invalid head\n");
    else if(ret==11) printf("[Heap validation] Invalid tail\n");
    return ret;
}

// Debug dump functions
void heap_dump_debug_information() {
    printf("\n***  HEAP INFO  ***\n");
    struct chunk_t *p = heap.head_chunk;
    int cnt=0;
    
    while(p) {
        printf("* Chunk %d:\n",++cnt);
        printf("- Chunk address: %p\n",p);
        printf("- Allocated: %d\n",p->alloc);
        printf("- Chunk size: %lu (incl. metadata: %lu)\n",p->size,p->size+sizeof(struct chunk_t));
        if(p->debug_line!=0)printf("- Debug fileline: %d\n",p->debug_line);
        if(p->debug_file!=NULL)printf("- Debug filename: %s\n",p->debug_file);
        printf("\n");
        p=p->next;
    }

    printf("* Heap data:\n");
    printf("- Heap size: %lu\n",heap_get_used_space()+heap_get_free_space());
    printf("- Bytes used: %lu\n",heap_get_used_space());
    printf("- Bytes free: %lu\n",heap_get_free_space());
    printf("- Largest free chunk to use: %lu\n",heap_get_largest_free_area());

    printf("\n*******************\n"); 
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

// Extra functions
struct chunk_t *heap_get_control_block(const void *pointer) {
    if(pointer==NULL) return NULL;
    if(get_pointer_type(pointer)!=pointer_valid) return NULL;
    return (struct chunk_t *)(pointer-sizeof(struct chunk_t));
}

struct heap_t *get_heap() {
    return &heap;
}


int main_old() {
    int tmp = heap_setup();

    printf("Used space: %lu\n",heap_get_used_space());
    printf("Largest used block: %lu\n", heap_get_largest_used_block_size());
    printf("Used blocks: %llu\n",heap_get_used_blocks_count());
    printf("Free space: %lu\n",heap_get_free_space());
    printf("Largest free area: %lu\n",heap_get_largest_free_area());
    printf("Free blocks: %llu\n",heap_get_free_gaps_count());

    /*struct chunk_t *p = heap_malloc(250);
    printf("\nAllocated 250 blocks\n\n");

    printf("Used space: %lu\n",heap_get_used_space());
    printf("Largest used block: %lu\n", heap_get_largest_used_block_size());
    printf("Used blocks: %llu\n",heap_get_used_blocks_count());
    printf("Free space: %lu\n",heap_get_free_space());
    printf("Largest free area: %lu\n",heap_get_largest_free_area());
    printf("Free blocks: %llu\n",heap_get_free_gaps_count());*/


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

    /*printf("Chunks: %d\n",heap.chunks);
    char *str = heap_malloc(50);
    strcpy(str,"Ala ma kota");
    printf("%s\n",str);
    printf("Chunks: %d\n",heap.chunks);
    printf("%lu\n", calc_size_in_page(heap.head_chunk->next));
    size_t dif = (heap.head_chunk->next - heap.head_chunk)-sizeof(struct chunk_t);
    printf("%lu\n",dif);
    printf("%lu %lu\n",heap.head_chunk->size,heap.head_chunk->next->size);
    char *str2 = heap_realloc(str,12);
    printf("%s\n%s\n",str,str2);
    printf("Chunks: %d\n",heap.chunks);
    dif = (heap.head_chunk->next - heap.head_chunk)-sizeof(struct chunk_t);
    printf("Test: %s %p\n",(char*)((char*)heap.head_chunk+sizeof(struct chunk_t)+2), str2);
    printf("%lu\n",dif);

    struct chunk_t *tt = heap_get_data_block_start((char*)heap.head_chunk+sizeof(struct chunk_t)+2);
    printf("Size of tt: %ld\n", tt->size);
    
    heap_free(str2);
    printf("Chunks: %d\n",heap.chunks);
    printf("%lu\n", calc_size_in_page(heap.head_chunk));*/

    char *str = heap_malloc_aligned_debug(50, __LINE__, __FILE__);
    if(str==NULL) printf("\nreturned NULL\n\n");
    else printf("\n\n");
    strcpy(str,"Ala ma kota\n");
    printf("String: %s\n",str);
    printf("Used space: %lu\n",heap_get_used_space());
    printf("Largest used block: %lu\n", heap_get_largest_used_block_size());
    printf("Used blocks: %llu\n",heap_get_used_blocks_count());
    printf("Free space: %lu\n",heap_get_free_space());
    printf("Largest free area: %lu\n",heap_get_largest_free_area());
    printf("Free blocks: %llu\n",heap_get_free_gaps_count());

    validate_and_print();
    heap_dump_debug_information();
    heap_free(str);
    heap_delete(0);

    return 0;
}

int main_old2() {
    heap_setup();
    print_pointer_type(heap.end_fence_p);
    void* p1 = heap_malloc(8 * 1024 * 1024); // 8MB
	void* p2 = heap_malloc(8 * 1024 * 1024); // 8MB
	void* p3 = heap_malloc(8 * 1024 * 1024); // 8MB
	void* p4 = heap_malloc(45 * 1024 * 1024); // 45MB
	assert(p1 != NULL); // malloc musi się udać
	assert(p2 != NULL); // malloc musi się udać
	assert(p3 != NULL); // malloc musi się udać
	assert(p4 == NULL); // nie ma prawa zadziałać
    heap_dump_debug_information();
    heap_free(p1);heap_free(p2);heap_free(p3);
    heap_delete(0);
    return 0;
}