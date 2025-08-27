// IDXTABLE
// ---
//
// The IDXTABLE module provides `IdxTable` object which resembles
// `std::unordered_map<int, T>` from C++ STL.
//
// NOTES
//
// - The `IdxTable` uses integers as keys. Negative integer keys are the same as
// positive, that is:
//
// ```pseudocode
// idxtable[100] == idxtable[-100];
// ```
//
// - Internally `IdxTable` uses a number of buckets to make lookup a little bit
// more efficient in large collections. The number of buckets is considered
// implementation detail and should not be changed. FYI it's 63 in Arcanum and
// 256 in ToEE.

#include "tig/idxtable.h"

#include <string.h>

#include "tig/file.h"
#include "tig/memory.h"

// Number of buckets allocated for each `TigIdxTable` object.
#define MAX_BUCKETS 63

// Magic 32 bit number in the file stream denoting start of serialized
// `TigIdxTable` representation.
#define START_SENTINEL 0xAB1EE1BAu

// Magic 32 bit number in the file stream denoting end of serialized
// `TigIdxTable` representation.
#define END_SENTINEL 0xE1BAAB1Eu

static void tig_idxtable_entry_destroy(TigIdxTableEntry* entry);
static TigIdxTableEntry* tig_idxtable_entry_create(int size);
static TigIdxTableEntry* tig_idxtable_entry_copy(TigIdxTableEntry* src, int size);
static int tig_idxtable_entry_count(TigIdxTableEntry* entry);
static bool tig_idxtable_entry_read(TigIdxTableEntry** entry_ptr, int size, int count, TigFile* stream);

// 0x534320
void tig_idxtable_init(TigIdxTable* idxtable, int value_size)
{
    int index;

    idxtable->buckets_count = MAX_BUCKETS;
    idxtable->buckets = (TigIdxTableEntry**)MALLOC(sizeof(idxtable->buckets) * MAX_BUCKETS);

    for (index = 0; index < idxtable->buckets_count; index++) {
        idxtable->buckets[index] = NULL;
    }

    idxtable->count = 0;
    idxtable->value_size = value_size;
}

// 0x534370
void tig_idxtable_exit(TigIdxTable* idxtable)
{
    int index;
    TigIdxTableEntry* entry;
    TigIdxTableEntry* next;

    for (index = 0; index < idxtable->buckets_count; index++) {
        // NOTE: Original code is slightly different.
        entry = idxtable->buckets[index];
        while (entry != NULL) {
            next = entry->next;
            tig_idxtable_entry_destroy(entry);
            entry = next;
        }
    }

    FREE(idxtable->buckets);
}

// 0x5343C0
void tig_idxtable_copy(TigIdxTable* dst, TigIdxTable* src)
{
    int index;

    dst->value_size = src->value_size;
    dst->buckets_count = src->buckets_count;
    dst->buckets = (TigIdxTableEntry**)MALLOC(sizeof(TigIdxTableEntry*) * src->buckets_count);

    for (index = 0; index < src->buckets_count; index++) {
        dst->buckets[index] = tig_idxtable_entry_copy(src->buckets[index], dst->value_size);
    }

    dst->count = src->count;
}

// 0x534430
size_t tig_idxtable_write(TigIdxTable* idxtable, TigFile* stream)
{
    unsigned int grd;
    int index;
    int count;
    TigIdxTableEntry* entry;
    size_t bytes_written = 0;

    grd = START_SENTINEL;
    bytes_written += tig_file_fwrite(&grd, sizeof(grd), 1, stream);

    bytes_written += tig_file_fwrite(&(idxtable->buckets_count), sizeof(idxtable->buckets_count), 1, stream);
    bytes_written += tig_file_fwrite(&(idxtable->value_size), sizeof(idxtable->value_size), 1, stream);

    for (index = 0; index < idxtable->buckets_count; index++) {
        count = tig_idxtable_entry_count(idxtable->buckets[index]);
        bytes_written += tig_file_fwrite(&count, sizeof(count), 1, stream);

        entry = idxtable->buckets[index];
        while (entry != NULL) {
            bytes_written += tig_file_fwrite(&(entry->key), sizeof(entry->key), 1, stream);
            bytes_written += tig_file_fwrite(entry->value, idxtable->value_size, 1, stream);
            entry = entry->next;
        }
    }

    grd = END_SENTINEL;
    bytes_written += tig_file_fwrite(&grd, sizeof(grd), 1, stream);

    return bytes_written;
}

// 0x534520
bool tig_idxtable_read(TigIdxTable* idxtable, TigFile* stream)
{
    unsigned int grd;
    int index;
    int count;
    TigIdxTableEntry* entry;
    TigIdxTableEntry* next;

    if (tig_file_fread(&grd, sizeof(grd), 1, stream) != sizeof(grd)) {
        return false;
    }

    if (grd != START_SENTINEL) {
        return false;
    }

    if (tig_file_fread(&(idxtable->buckets_count), sizeof(idxtable->buckets_count), 1, stream) != sizeof(idxtable->buckets_count)) {
        return false;
    }

    if (tig_file_fread(&(idxtable->value_size), sizeof(idxtable->value_size), 1, stream) != sizeof(idxtable->value_size)) {
        return false;
    }

    idxtable->count = 0;

    for (index = 0; index < idxtable->buckets_count; index++) {
        entry = idxtable->buckets[index];
        while (entry != NULL) {
            next = entry->next;
            tig_idxtable_entry_destroy(entry);
            entry = next;
        }

        if (tig_file_fread(&count, sizeof(count), 1, stream) != sizeof(count)) {
            return false;
        }

        if (!tig_idxtable_entry_read(&(idxtable->buckets[index]), idxtable->value_size, count, stream)) {
            return false;
        }

        idxtable->count += count;
    }

    if (tig_file_fread(&grd, sizeof(grd), 1, stream) != sizeof(grd)) {
        return false;
    }

    if (grd != END_SENTINEL) {
        return false;
    }

    return true;
}

// 0x534650
bool tig_idxtable_enumerate(TigIdxTable* idxtable, TigIdxTableEnumerateCallback* callback, void* context)
{
    int index;
    TigIdxTableEntry* entry;

    for (index = 0; index < idxtable->buckets_count; index++) {
        entry = idxtable->buckets[index];
        while (entry != NULL) {
            if (!callback(entry->value, entry->key, context)) {
                return false;
            }
            entry = entry->next;
        }
    }

    return true;
}

// 0x5346A0
void tig_idxtable_set(TigIdxTable* idxtable, int key, void* value)
{
    int bucket;
    TigIdxTableEntry* entry;

    if (key < 0) {
        key = -key;
    }

    bucket = key % idxtable->buckets_count;
    entry = idxtable->buckets[bucket];
    while (entry != NULL) {
        if (entry->key == key) {
            memcpy(entry->value, value, idxtable->value_size);
            return;
        }
        entry = entry->next;
    }

    entry = tig_idxtable_entry_create(idxtable->value_size);
    memcpy(entry->value, value, idxtable->value_size);
    entry->key = key;
    entry->next = idxtable->buckets[bucket];
    idxtable->buckets[bucket] = entry;

    idxtable->count++;
}

// 0x534730
bool tig_idxtable_get(TigIdxTable* idxtable, int key, void* value)
{
    int bucket;
    TigIdxTableEntry* entry;

    if (key < 0) {
        key = -key;
    }

    bucket = key % idxtable->buckets_count;
    entry = idxtable->buckets[bucket];
    while (entry != NULL) {
        if (entry->key == key) {
            memcpy(value, entry->value, idxtable->value_size);
            return true;
        }
        entry = entry->next;
    }

    return false;
}

// 0x534780
bool tig_idxtable_contains(TigIdxTable* idxtable, int key)
{
    int bucket;
    TigIdxTableEntry* entry;

    if (key < 0) {
        key = -key;
    }

    bucket = key % idxtable->buckets_count;
    entry = idxtable->buckets[bucket];
    while (entry != NULL) {
        if (entry->key == key) {
            return true;
        }
        entry = entry->next;
    }

    return false;
}

// 0x5347C0
void tig_idxtable_remove(TigIdxTable* idxtable, int key)
{
    int bucket;
    TigIdxTableEntry** entry_ptr;
    TigIdxTableEntry* entry;

    if (key < 0) {
        key = -key;
    }

    // NOTE: Original implementation is different.
    bucket = key % idxtable->buckets_count;
    entry_ptr = &(idxtable->buckets[bucket]);

    while (*entry_ptr != NULL) {
        entry = *entry_ptr;
        if (entry->key == key) {
            *entry_ptr = entry->next;
            tig_idxtable_entry_destroy(entry);
            idxtable->count--;
            break;
        }
        entry_ptr = &(entry->next);
    }
}

// 0x534840
void tig_idxtable_entry_destroy(TigIdxTableEntry* entry)
{
    FREE(entry->value);
    FREE(entry);
}

// 0x534860
TigIdxTableEntry* tig_idxtable_entry_create(int size)
{
    TigIdxTableEntry* entry;

    entry = (TigIdxTableEntry*)MALLOC(sizeof(*entry));
    entry->value = MALLOC(size);

    return entry;
}

// 0x534880
TigIdxTableEntry* tig_idxtable_entry_copy(TigIdxTableEntry* entry, int size)
{
    TigIdxTableEntry* copy;

    if (entry == NULL) {
        return NULL;
    }

    copy = tig_idxtable_entry_create(size);
    memcpy(copy->value, entry->value, size);
    copy->next = tig_idxtable_entry_copy(entry->next, size);

    // BUGFIX: Key was not being copied.
    copy->key = entry->key;

    return copy;
}

// 0x5348D0
int tig_idxtable_entry_count(TigIdxTableEntry* entry)
{
    return entry != NULL ? tig_idxtable_entry_count(entry->next) + 1 : 0;
}

// 0x5348F0
bool tig_idxtable_entry_read(TigIdxTableEntry** entry_ptr, int size, int count, TigFile* stream)
{
    int index;

    for (index = 0; index < count; index++) {
        *entry_ptr = tig_idxtable_entry_create(size);
        // FIX: Original code expects return value to be `4`. This is wrong as
        // `fread` is supposed to return number of elements read, not number of
        // bytes.
        if (tig_file_fread(&((*entry_ptr)->key), sizeof((*entry_ptr)->key), 1, stream) != 1) {
            tig_idxtable_entry_destroy(*entry_ptr);
            return false;
        }

        // FIX: Original code expects return value to be `size`. Same error as
        // above.
        if (tig_file_fread(&((*entry_ptr)->value), size, 1, stream) != 1) {
            tig_idxtable_entry_destroy(*entry_ptr);
            return false;
        }
        entry_ptr++;
    }

    *entry_ptr = NULL;
    return true;
}

// 0x534990
int tig_idxtable_count(TigIdxTable* idxtable)
{
    return idxtable->count;
}
