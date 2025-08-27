#include "tig/memory.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define START_GUARD_BYTE 0xAA
#define START_GUARD_SIZE ((int)sizeof(void*))
#define END_GUARD_BYTE 0xBB
#define END_GUARD_SIZE ((int)sizeof(void*))

typedef struct TigMemoryBlock {
    /* 0000 */ void* data;
    /* 0004 */ size_t size;
    /* 0008 */ const char* file;
    /* 000C */ int line;
    /* 0010 */ struct TigMemoryBlock* next;
} TigMemoryBlock;

// Size of block plus a pair of guards.
#define OVERHEAD_SIZE (sizeof(TigMemoryBlock) + START_GUARD_SIZE + END_GUARD_SIZE)

static int tig_memory_sort_blocks(const void* a1, const void* a2);
static void tig_memory_fatal_error(const char* format, ...);
static void tig_memory_validate(TigMemoryBlock* block, const char* file, int line);

// 0x603DE8
static size_t tig_memory_max_overhead;

// 0x603DEC
static size_t tig_memory_max_blocks;

// 0x603DF0
static size_t tig_memory_current_overhead;

// 0x603DF4
static size_t tig_memory_max_allocated;

// 0x603DF8
static size_t tig_memory_current_allocated;

// 0x603DFC
static size_t tig_memory_current_blocks;

// 0x0603E00
static TigMemoryOutputFunc* tig_memory_output_func;

// 0x603E04
static char tig_memory_output_buffer[1024];

// 0x604204
static TigMemoryBlock* tig_memory_blocks_head;

// 0x604208
static bool tig_memory_initialized;

// 0x739F40
static SDL_Mutex* tig_memory_mutex;

// 0x4FE380
int tig_memory_init(TigInitInfo* init_info)
{
    (void)init_info;

    tig_memory_mutex = SDL_CreateMutex();

    tig_memory_initialized = true;

    return TIG_OK;
}

// 0x4FE3A0
void tig_memory_exit(void)
{
    SDL_DestroyMutex(tig_memory_mutex);
    tig_memory_mutex = NULL;

    tig_memory_initialized = false;
}

// 0x4FE3C0
void* tig_memory_calloc(size_t count, size_t size, const char* file, int line)
{
    void* ptr;

    if (!tig_memory_initialized) {
        tig_memory_init(NULL);
    }

    SDL_LockMutex(tig_memory_mutex);

    ptr = tig_memory_alloc(size * count, file, line);
    if (ptr != NULL) {
        memset(ptr, 0, size * count);
    }

    SDL_UnlockMutex(tig_memory_mutex);

    return ptr;
}

// 0x4FE430
void tig_memory_free(void* ptr, const char* file, int line)
{
    TigMemoryBlock* block;
    TigMemoryBlock* prev;

    if (!tig_memory_initialized) {
        tig_memory_init(NULL);
    }

    SDL_LockMutex(tig_memory_mutex);

    block = tig_memory_blocks_head;
    prev = NULL;
    while (block != NULL && block->data != ptr) {
        prev = block;
        block = block->next;
    }

    if (block == NULL) {
        // NOTE: Format is slightly modified for VS Code to recognize file path.
        tig_memory_fatal_error("TIG Memory: Error - unable to locate block to free in %s:%d.",
            file,
            line);
    }

    tig_memory_validate(block, file, line);

    tig_memory_current_blocks -= 1;
    tig_memory_current_allocated -= block->size;
    tig_memory_current_overhead -= OVERHEAD_SIZE;

    if (prev != NULL) {
        prev->next = block->next;
    } else {
        tig_memory_blocks_head = block->next;
    }

    free(block);

    SDL_UnlockMutex(tig_memory_mutex);
}

// 0x4FE500
void* tig_memory_alloc(size_t size, const char* file, int line)
{
    void* ptr;
    TigMemoryBlock* block;

    if (!tig_memory_initialized) {
        tig_memory_init(NULL);
    }

    SDL_LockMutex(tig_memory_mutex);

    ptr = malloc(size + OVERHEAD_SIZE);
    if (ptr == NULL) {
        // NOTE: Format is slightly modified for VS Code to recognize file path.
        tig_memory_fatal_error("TIG Memory: Error - unable to allocate block of %zu bytes in %s:%d.",
            size,
            file,
            line);
    }

    block = (TigMemoryBlock*)ptr;
    block->data = (unsigned char*)ptr + sizeof(TigMemoryBlock) + START_GUARD_SIZE;
    block->size = size;
    block->file = file;
    block->line = line;
    block->next = tig_memory_blocks_head;
    tig_memory_blocks_head = block;

    // NOTE: Original code does not use `memset`, but it's a little bit safer
    // when you consider alignment issues.
    memset((unsigned char*)block->data - START_GUARD_SIZE, START_GUARD_BYTE, START_GUARD_SIZE);
    memset((unsigned char*)block->data + block->size, END_GUARD_BYTE, END_GUARD_SIZE);

    tig_memory_current_blocks += 1;
    tig_memory_current_overhead += OVERHEAD_SIZE;
    tig_memory_current_allocated += size;

    if (tig_memory_max_blocks < tig_memory_current_blocks) {
        tig_memory_max_blocks = tig_memory_current_blocks;
    }

    if (tig_memory_max_allocated < tig_memory_current_allocated) {
        tig_memory_max_allocated = tig_memory_current_allocated;
    }

    if (tig_memory_max_overhead < tig_memory_current_overhead) {
        tig_memory_max_overhead = tig_memory_current_overhead;
    }

    SDL_UnlockMutex(tig_memory_mutex);

    return block->data;
}

// 0x4FE5F0
void* tig_memory_realloc(void* ptr, size_t size, const char* file, int line)
{
    TigMemoryBlock* block;
    TigMemoryBlock* prev;
    size_t old_size;

    if (!tig_memory_initialized) {
        tig_memory_init(NULL);
    }

    if (ptr == NULL) {
        return tig_memory_alloc(size, file, line);
    }

    SDL_LockMutex(tig_memory_mutex);

    block = tig_memory_blocks_head;
    prev = NULL;
    while (block != NULL && block->data != ptr) {
        prev = block;
        block = block->next;
    }

    if (block == NULL) {
        // NOTE: Format is slightly modified for VS Code to recognize file path.
        tig_memory_fatal_error("TIG Memory: Error - unable to locate block to reallocate in %s:%d.",
            file,
            line);
    }

    if (prev != NULL) {
        prev->next = block->next;
    } else {
        tig_memory_blocks_head = block->next;
    }

    old_size = block->size;

    ptr = realloc(block, size + OVERHEAD_SIZE);
    if (ptr == NULL) {
        // NOTE: Format is slightly modified for VS Code to recognize file path.
        tig_memory_fatal_error("TIG Memory: Error - unable to reallocate block of %zu bytes in %s:%d.",
            size,
            file,
            line);
    }

    block = (TigMemoryBlock*)ptr;
    block->data = (unsigned char*)ptr + sizeof(TigMemoryBlock) + START_GUARD_SIZE;
    block->size = size;
    block->file = file;
    block->line = line;
    block->next = tig_memory_blocks_head;
    tig_memory_blocks_head = block;

    // NOTE: Start guard stays in tact.
    memset((unsigned char*)block->data + size, END_GUARD_BYTE, END_GUARD_SIZE);

    tig_memory_current_allocated += size - old_size;

    if (tig_memory_max_allocated < tig_memory_current_allocated) {
        tig_memory_max_allocated = tig_memory_current_allocated;
    }

    SDL_UnlockMutex(tig_memory_mutex);

    return block->data;
}

// 0x4FE710
char* tig_memory_strdup(const char* str, const char* file, int line)
{
    char* copy;

    if (!tig_memory_initialized) {
        tig_memory_init(NULL);
    }

    SDL_LockMutex(tig_memory_mutex);

    copy = (char*)tig_memory_alloc(strlen(str) + 1, file, line);
    strcpy(copy, str);

    SDL_UnlockMutex(tig_memory_mutex);

    return copy;
}

// 0x4FE790
void tig_memory_set_output_func(TigMemoryOutputFunc* func)
{
    tig_memory_output_func = func;
}

// 0x4FE7A0
void tig_memory_print_stats(TigMemoryPrintStatsOptions opts)
{
    if (tig_memory_output_func == NULL) {
        return;
    }

    tig_memory_output_func("\n--- Dynamic memory usage: ---");

    if ((opts & TIG_MEMORY_STATS_PRINT_OVERHEAD) != 0) {
        sprintf(tig_memory_output_buffer,
            "Peak memory management overhead: %zu bytes.",
            tig_memory_max_overhead);
        tig_memory_output_func(tig_memory_output_buffer);

        sprintf(tig_memory_output_buffer,
            "Current memory management overhead: %zu bytes.",
            tig_memory_current_overhead);
        tig_memory_output_func(tig_memory_output_buffer);
    }

    sprintf(tig_memory_output_buffer,
        "Peak program memory usage: %zu blocks, %zu bytes.",
        tig_memory_max_blocks,
        tig_memory_max_allocated);
    tig_memory_output_func(tig_memory_output_buffer);

    sprintf(tig_memory_output_buffer,
        "Current program memory usage: %zu blocks totaling %zu bytes.",
        tig_memory_current_blocks,
        tig_memory_current_allocated);
    tig_memory_output_func(tig_memory_output_buffer);

    if ((opts & TIG_MEMORY_STATS_PRINT_ALL_BLOCKS) != 0) {
        TigMemoryBlock* curr;

        curr = tig_memory_blocks_head;
        while (curr != NULL) {
            // NOTE: Format is slightly modified for VS Code to recognize file
            // path. In addition %08x is replaced with %p to prevent compiler
            // warning.
            sprintf(tig_memory_output_buffer,
                "    %s:%d:  %zu bytes at %p.",
                curr->file,
                curr->line,
                curr->size,
                curr->data);
            tig_memory_output_func(tig_memory_output_buffer);
            curr = curr->next;
        }
    } else if ((opts & TIG_MEMORY_STATS_PRINT_GROUPED_BLOCKS) != 0) {
        TigMemoryBlock** array;
        TigMemoryBlock* curr;
        size_t index;
        size_t allocated;
        size_t blocks;

        array = (TigMemoryBlock**)malloc(sizeof(TigMemoryBlock*) * tig_memory_current_blocks);

        index = 0;
        curr = tig_memory_blocks_head;
        while (curr != NULL) {
            array[index++] = curr;
            curr = curr->next;
        }

        qsort(array, tig_memory_current_blocks, sizeof(blocks), tig_memory_sort_blocks);

        allocated = 0;
        blocks = 0;
        for (index = 0; index < tig_memory_current_blocks; index++) {
            allocated += array[index]->size;
            blocks++;

            if (index == tig_memory_current_blocks - 1
                || SDL_strcasecmp(array[index]->file, array[index + 1]->file) != 0
                || array[index]->line != array[index + 1]->line) {
                // NOTE: Format is slightly modified for VS Code to recognize
                // file path.
                sprintf(tig_memory_output_buffer,
                    "    %s:%d:  %zu blocks totaling %zu bytes.",
                    array[index]->file,
                    array[index]->line,
                    blocks,
                    allocated);
                tig_memory_output_func(tig_memory_output_buffer);
                allocated = 0;
                blocks = 0;
            }
        }

        free(array);
    }
}

// 0x4FE990
int tig_memory_sort_blocks(const void* a1, const void* a2)
{
    TigMemoryBlock* block1 = *(TigMemoryBlock**)a1;
    TigMemoryBlock* block2 = *(TigMemoryBlock**)a2;
    int cmp;

    cmp = SDL_strcasecmp(block1->file, block2->file);
    if (cmp == 0) {
        cmp = block1->line - block2->line;
    }

    return cmp;
}

// 0x4FE9D0
void tig_memory_validate_all(const char* file, int line)
{
    TigMemoryBlock* block;

    if (!tig_memory_initialized) {
        tig_memory_init(NULL);
    }

    SDL_LockMutex(tig_memory_mutex);

    block = tig_memory_blocks_head;
    while (block != NULL) {
        tig_memory_validate(block, file, line);
        block = block->next;
    }

    SDL_UnlockMutex(tig_memory_mutex);
}

// 0x4FEA30
void tig_memory_get_system_status(size_t* total, size_t* available)
{
    // SDL exposes installed RAM in megabytes, but the original API had this
    // value in bytes.
    *total = (size_t)SDL_GetSystemRAM() * 1024 * 1024;

    // SDL does not have API to get available RAM.
    *available = *total;
}

// 0x4FEA60
void tig_memory_fatal_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    if (tig_memory_output_func != NULL) {
        vsprintf(tig_memory_output_buffer, format, args);
        tig_memory_output_func(tig_memory_output_buffer);
    }

    va_end(args);

    exit(EXIT_FAILURE);
}

// 0x4FEAA0
void tig_memory_validate(TigMemoryBlock* block, const char* file, int line)
{
    unsigned char* grd;
    int index;

    // NOTE: Validation code checks byte-by-byte, so it's alignment-safe.
    grd = (unsigned char*)block->data - START_GUARD_SIZE;
    for (index = 0; index < START_GUARD_SIZE; index++) {
        if (*grd != START_GUARD_BYTE) {
            // NOTE: Format is slightly modified for VS Code to recognize file
            // path.
            tig_memory_fatal_error("TIG Memory: Error - overwrite detected in starting guard byte %d for block allocated in %s:%d.  Error detected in %s:%d.",
                index,
                block->file,
                block->line,
                file,
                line);
        }
    }

    grd = (unsigned char*)block->data + block->size;
    for (index = 0; index < END_GUARD_SIZE; index++) {
        if (*grd != END_GUARD_BYTE) {
            // NOTE: Format is slightly modified for VS Code to recognize file
            // path.
            tig_memory_fatal_error("TIG Memory: Error - overwrite detected in ending guard byte %d for block allocated in %s:%d.  Error detected in %s:%d.",
                index,
                block->file,
                block->line,
                file,
                line);
        }
    }
}

#ifndef NDEBUG

void tig_memory_stats(TigMemoryStats* stats)
{
    stats->current_allocated = tig_memory_current_allocated;
    stats->current_blocks = tig_memory_current_blocks;
    stats->max_allocated = tig_memory_max_allocated;
    stats->max_blocks = tig_memory_max_blocks;
}

void tig_memory_reset_stats()
{
    tig_memory_current_overhead = 0;
    tig_memory_current_allocated = 0;
    tig_memory_current_blocks = 0;
    tig_memory_max_overhead = 0;
    tig_memory_max_allocated = 0;
    tig_memory_max_blocks = 0;
}

static void validate_memory_leaks_output_callback(const char* str)
{
    printf("%s\n", str);
}

bool tig_memory_validate_memory_leaks()
{
    TigMemoryOutputFunc* fn;

    if (tig_memory_current_blocks == 0 || tig_memory_current_allocated == 0) {
        return true;
    }

    fn = tig_memory_output_func;
    tig_memory_output_func = validate_memory_leaks_output_callback;
    tig_memory_print_stats(TIG_MEMORY_STATS_PRINT_OVERHEAD | TIG_MEMORY_STATS_PRINT_GROUPED_BLOCKS);
    tig_memory_output_func = fn;

    return false;
}

#endif
