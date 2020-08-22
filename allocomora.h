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

int heap_setup();
void *heap_malloc_debug(size_t count, int fileline, const char* filename);
void *find_free_chunk(size_t size);
void update_heap_data();
void update_end_fence();
struct chunk_t *merge(struct chunk_t chunk1, struct chunk_t chunk2);
struct chunk_t *split(struct chunk_t *chunk_to_split, size_t size);

//change test vscode 
#endif //ALLOCOMORA_H