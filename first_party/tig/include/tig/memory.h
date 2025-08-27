#ifndef TIG_MEMORY_H_
#define TIG_MEMORY_H_

#include <stdlib.h>

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Signature of function used during `tig_memory_print_stats`.
typedef void(TigMemoryOutputFunc)(const char*);

// Level of details during `tig_memory_print_stats`.
//
// `TIG_MEMORY_STATS_PRINT_ALL_BLOCKS` and
// `TIG_MEMORY_STATS_PRINT_GROUPED_BLOCKS` are mutually exclusive.
typedef enum TigMemoryPrintStatsOptions {
    TIG_MEMORY_STATS_PRINT_OVERHEAD = 1 << 0,
    TIG_MEMORY_STATS_PRINT_ALL_BLOCKS = 1 << 1,
    TIG_MEMORY_STATS_PRINT_GROUPED_BLOCKS = 1 << 2,
} TigMemoryPrintStatsOptions;

int tig_memory_init(TigInitInfo* init_info);
void tig_memory_exit(void);
void* tig_memory_calloc(size_t count, size_t size, const char* file, int line);
void tig_memory_free(void* ptr, const char* file, int line);
void* tig_memory_alloc(size_t size, const char* file, int line);
void* tig_memory_realloc(void* ptr, size_t size, const char* file, int line);
char* tig_memory_strdup(const char* str, const char* file, int line);
void tig_memory_set_output_func(TigMemoryOutputFunc* func);
void tig_memory_print_stats(TigMemoryPrintStatsOptions opts);
void tig_memory_validate_all(const char* file, int line);
void tig_memory_get_system_status(size_t* total, size_t* available);

// NOTE: Unlike Fallouts, where entire code base use similar functionality to
// manage memory, allocation functions of this module are never used (at least
// in Arcanum, one call in ToEE doesn't count). However, their presence imply
// they were probably used during development, so should we.
#ifdef TIG_DEBUG_MEMORY
#define MALLOC(size) tig_memory_alloc(size, __FILE__, __LINE__)
#define REALLOC(ptr, size) tig_memory_realloc(ptr, size, __FILE__, __LINE__)
#define FREE(ptr) tig_memory_free(ptr, __FILE__, __LINE__)
#define CALLOC(count, size) tig_memory_calloc(count, size, __FILE__, __LINE__)
#define STRDUP(str) tig_memory_strdup(str, __FILE__, __LINE__)
#else
#define MALLOC(size) malloc(size)
#define REALLOC(ptr, size) realloc(ptr, size)
#define FREE(ptr) free(ptr)
#define CALLOC(count, size) calloc(count, size)
#define STRDUP(str) strdup(str)
#endif

// Testing.
#ifndef NDEBUG

typedef struct TigMemoryStats {
    size_t current_allocated;
    size_t current_blocks;
    size_t max_allocated;
    size_t max_blocks;
} TigMemoryStats;

void tig_memory_stats(TigMemoryStats* stats);
void tig_memory_reset_stats();
bool tig_memory_validate_memory_leaks();

#endif

#ifdef __cplusplus
}
#endif

#endif /* TIG_MEMORY_H_ */
