#ifndef ALLOCOMORA_H
#define ALLOCOMORA_H

#define KB 1024
#define PAGE_SIZE (4*KB)
#define PAGES_BGN 1
#define FIRFENCE 369258303
#define SECFENCE 495105411
#define LASFENCE 693452304

struct chunk_t {
    int first_fence;
    struct chunk_t *prev;
    size_t size;
    struct chunk_t *next;
    int debug_line;
    const char *debug_file;
    char alloc;
    int checksum;
    int second_fence;
};

struct heap_t {
    struct chunk_t *head_chunk;
    struct chunk_t *tail_chunk;
    int *end_fence_p;
    char is_set;
    void *data;
    //size_t size;
    int pages;
    int chunks;
};

enum pointer_type_t
{
      pointer_null,
      pointer_out_of_heap,
      pointer_control_block,
      pointer_inside_data_block,
      pointer_unallocated,
      pointer_valid,
      pointer_end_fence // if get_pointer_type returns it, pointer points at the end fence (int at the end of the heap).
};

int heap_setup();
void *heap_malloc_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename); 
void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename);
void *heap_malloc(size_t count);
void *heap_calloc(size_t number, size_t size);
void *heap_realloc(void* memblock, size_t size);
void heap_free(void* memblock);
void *find_free_chunk(size_t size);
enum pointer_type_t get_pointer_type(const void* pointer);
void update_heap_data();
void update_end_fence();
struct chunk_t *merge(struct chunk_t *chunk1, struct chunk_t *chunk2, char safe_mode);
struct chunk_t *split(struct chunk_t *chunk_to_split, size_t size);

//change test vscode 
#endif //ALLOCOMORA_H