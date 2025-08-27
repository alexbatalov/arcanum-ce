#ifndef TIG_IDXTABLE_H_
#define TIG_IDXTABLE_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TigFile TigFile;

typedef struct TigIdxTable TigIdxTable;
typedef struct TigIdxTableEntry TigIdxTableEntry;

// A key/value pair representing entry in a `TigIdxTable`.
//
// This type is private (visible for testing).
typedef struct TigIdxTableEntry {
    /* 0000 */ void* value;
    /* 0004 */ TigIdxTableEntry* next;
    /* 0008 */ int key;
} TigIdxTableEntry;

// A collection whose elements are key/value pairs.
typedef struct TigIdxTable {
    /* 0000 */ int buckets_count;
    /* 0004 */ TigIdxTableEntry** buckets;
    /* 0008 */ int value_size;
    /* 000C */ int count;
} TigIdxTable;

// Signature of a callback used by `tig_idxtable_enumerate`.
//
// The callback should return `true` to continue enumeration, or `false` to
// stop it.
typedef bool(TigIdxTableEnumerateCallback)(void* value, int key, void* context);

void tig_idxtable_init(TigIdxTable* idxtable, int value_size);
void tig_idxtable_exit(TigIdxTable* idxtable);
void tig_idxtable_copy(TigIdxTable* dst, TigIdxTable* src);
size_t tig_idxtable_write(TigIdxTable* idxtable, TigFile* stream);
bool tig_idxtable_read(TigIdxTable* idxtable, TigFile* stream);
bool tig_idxtable_enumerate(TigIdxTable* idxtable, TigIdxTableEnumerateCallback* callback, void* context);
void tig_idxtable_set(TigIdxTable* idxtable, int key, void* value);
bool tig_idxtable_get(TigIdxTable* idxtable, int key, void* value);
bool tig_idxtable_contains(TigIdxTable* idxtable, int key);
void tig_idxtable_remove(TigIdxTable* idxtable, int key);
int tig_idxtable_count(TigIdxTable* idxtable);

#ifdef __cplusplus
}
#endif

#endif /* TIG_IDXTABLE_H_ */
