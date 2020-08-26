#ifndef ALLOCOMORA_H
#define ALLOCOMORA_H

// Basic data
#define KB 1024
#define MB (1024*1024)
#define PAGE_SIZE (4*KB)
#define PAGES_BGN 2
#define FIRFENCE 369258303
#define SECFENCE 495105411
#define LASFENCE 693452304

// Debug options
#define LOG 1
#define TESTING 1

// Structures
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
    int pages;
    int chunks;
    int checksum;
};

// Enums
enum pointer_type_t {
      pointer_null,
      pointer_out_of_heap,
      pointer_control_block,
      pointer_inside_data_block,
      pointer_unallocated,
      pointer_valid,
      pointer_end_fence // if get_pointer_type returns it, pointer points at the end fence (int at the end of the heap).
};

enum validation_code_t {
    no_errors,
    err_heap_checksum,
    err_head_is_null,
    err_tail_is_null,
    err_end_fence,
    err_chunk_fence1,
    err_chunk_fence2,
    err_chunk_checksum,
    err_invalid_prev,
    err_invalid_next,
    err_invalid_head,
    err_invalid_tail
};

// Heap basic functions
int heap_setup();
int heap_delete(int safe_mode);
int heap_reset();

// *alloc functions
void *heap_malloc_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename); 
void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename);

// *alloc_aligned functions
void *heap_malloc_aligned_debug(size_t count, int fileline, const char *filename);
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename); 
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline, const char* filename);

// Non-debug *alloc and *alloc_aligned functions
void *heap_malloc(size_t count);
void *heap_calloc(size_t number, size_t size);
void *heap_realloc(void* memblock, size_t size);
void *heap_malloc_aligned(size_t count);
void *heap_calloc_aligned(size_t number, size_t size);
void *heap_realloc_aligned(void* memblock, size_t size);

// Chunk management functions
void heap_free(void* memblock);
struct chunk_t *merge(struct chunk_t *chunk1, struct chunk_t *chunk2, char safe_mode);
struct chunk_t *split(struct chunk_t *chunk_to_split, size_t size);
void *find_free_chunk(size_t size);

// Heap control functions
enum pointer_type_t get_pointer_type(const void* pointer);
void update_heap_data();
void update_end_fence();

// Statistics functions
void* heap_get_data_block_start(const void* pointer);
size_t heap_get_used_space(void);
size_t heap_get_largest_used_block_size(void);
uint64_t heap_get_used_blocks_count(void); 
size_t heap_get_free_space(void);
size_t heap_get_largest_free_area(void);
uint64_t heap_get_free_gaps_count(void);
size_t heap_get_block_size(const void* memblock);

// Checksum functions
void update_chunk_checksum(struct chunk_t *chunk);
void update_heap_checksum();
int verify_chunk_checksum(struct chunk_t *chunk);
int verify_heap_checksum();

// Calculation functions
size_t calc_dist(struct chunk_t *chunk);
size_t calc_size_in_page(struct chunk_t *chunk, size_t dist);
int is_aligned(size_t dist);

// Validation functions
enum validation_code_t heap_validate(void);
enum validation_code_t validate_and_print();

// Debug dump functions
void heap_dump_debug_information(void);
void print_pointer_type(const void* pointer);

// Extra functions
struct chunk_t *heap_get_control_block(const void *pointer);
struct heap_t *get_heap();

#endif //ALLOCOMORA_H