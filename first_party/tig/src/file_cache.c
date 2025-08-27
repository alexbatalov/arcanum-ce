// FILECACHE
//
// The FILECACHE module provides `TigFileCache` object used to cache files
// loaded from FILE module.
//
// Implements a least-recently-used cache. The maximum number of files and
// maximum the cache can hold is provided during creation.
//
// NOTES
//
// - Unlike many other subsystems which track allocations, file cache objects
// are not tracked at subsystem level. Which in turn means `tig_file_cache_exit`
// does not release existing cache objects. You're responsible for releasing of
// file cache objects with `tig_file_cache_destroy` before calling
// `tig_file_cache_exit`.
//
// - The `TigFileCache` resembles `Cache` object from Fallouts, but it`s API
// and implementation is much simpler.
//
// - There is only one use case in both Arcanum and ToEE - caching sound files
// (see SOUND subsystem). This is different from Fallouts where `Cache` was also
// used for art files. In TIG the ART subsystem has it's own cache, which is
// considered implementation detail and have no public API.

#include "tig/file_cache.h"

#include "tig/file.h"
#include "tig/memory.h"

#define FOURCC_FILC SDL_FOURCC('C', 'L', 'I', 'F')

static void tig_file_cache_entry_remove(TigFileCache* cache, TigFileCacheItem* entry);
static bool tig_file_cache_read_contents_into(const char* path, void** data, int* size);
static bool tig_file_cache_prepare_item(TigFileCache* cache, TigFileCacheItem* item, const char* path);
static TigFileCacheItem* sub_538D80(TigFileCache* cache);
static TigFileCacheItem* sub_538D90(TigFileCache* cache);
static TigFileCacheItem* sub_538E00(TigFileCache* cache);
static void tig_file_cache_shrink(TigFileCache* cache, int size);
static TigFileCacheEntry* tig_file_cache_acquire_internal(TigFileCache* cache, TigFileCacheItem* item);
static void tig_file_cache_release_internal(TigFileCache* cache, TigFileCacheItem* entry);

// 0x6364F8
static int tig_file_cache_hit_count;

// 0x6364FC
static int tig_file_cache_miss_count;

// 0x636500
static int tig_file_cache_hit_bytes;

// 0x636504
static int tig_file_cache_miss_bytes;

// 0x636508
static int tig_file_cache_removed_bytes;

// 0x538A80
int tig_file_cache_init(TigInitInfo* init_info)
{
    (void)init_info;

    return TIG_OK;
}

// 0x538A90
void tig_file_cache_exit()
{
}

// 0x538AA0
void tig_file_cache_flush(TigFileCache* cache)
{
    int index;

    for (index = 0; index < cache->capacity; index++) {
        if (cache->items[index].refcount == 0) {
            tig_file_cache_entry_remove(cache, &(cache->items[index]));
        }
    }
}

// 0x538AE0
void tig_file_cache_entry_remove(TigFileCache* cache, TigFileCacheItem* item)
{
    if (item->entry.data != NULL) {
        cache->items_count--;
        cache->bytes -= item->entry.size;
        tig_file_cache_removed_bytes += item->entry.size;

        if (item->entry.path) {
            FREE(item->entry.path);
            item->entry.path = NULL;
        }

        FREE(item->entry.data);
        item->entry.data = NULL;

        memset(item, 0, sizeof(*item));
    }
}

// 0x538B50
TigFileCache* tig_file_cache_create(int capacity, int max_size)
{
    TigFileCache* cache;

    cache = (TigFileCache*)MALLOC(sizeof(*cache));
    cache->signature = FOURCC_FILC;
    cache->capacity = capacity;
    cache->max_size = max_size;
    cache->items = (TigFileCacheItem*)CALLOC(sizeof(*cache->items), capacity);
    cache->items_count = 0;
    cache->bytes = 0;
    return cache;
}

// 0x538BA0
void tig_file_cache_destroy(TigFileCache* cache)
{
    tig_file_cache_flush(cache);
    FREE(cache->items);
    FREE(cache);
}

// 0x538BC0
bool tig_file_cache_read_contents_into(const char* path, void** data, int* size)
{
    TigFile* stream;

    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }

    *size = tig_file_filelength(stream);
    *data = MALLOC(*size);
    tig_file_fread(*data, *size, 1, stream);
    tig_file_fclose(stream);

    return true;
}

// 0x538C20
bool tig_file_cache_prepare_item(TigFileCache* cache, TigFileCacheItem* item, const char* path)
{
    if (!tig_file_cache_read_contents_into(path, &(item->entry.data), &(item->entry.size))) {
        return false;
    }

    item->entry.path = STRDUP(path);

    cache->items_count++;
    cache->bytes += item->entry.size;
    item->entry.index = item - cache->items;

    return true;
}

// 0x538C90
TigFileCacheEntry* tig_file_cache_acquire(TigFileCache* cache, const char* path)
{
    // 0x6364E8
    static TigFileCacheEntry null_entry;

    int index;
    TigFileCacheItem* item;
    TigFileCacheItem* new_item = NULL;
    TigFileCacheEntry* entry;

    for (index = 0; index < cache->capacity; index++) {
        item = &(cache->items[index]);
        if (item->entry.data != NULL) {
            if (SDL_strcasecmp(item->entry.path, path) == 0) {
                tig_file_cache_hit_count++;
                tig_file_cache_hit_bytes += item->entry.size;
                return tig_file_cache_acquire_internal(cache, item);
            }
        } else {
            if (new_item == NULL) {
                new_item = item;
            }
        }
    }

    if (new_item == NULL) {
        new_item = sub_538D80(cache);
        if (new_item == NULL) {
            return &null_entry;
        }
    }

    if (!tig_file_cache_prepare_item(cache, new_item, path)) {
        return &null_entry;
    }

    tig_file_cache_miss_count++;
    tig_file_cache_miss_bytes += new_item->entry.size;

    entry = tig_file_cache_acquire_internal(cache, new_item);

    if (cache->bytes > cache->max_size) {
        tig_file_cache_shrink(cache, cache->bytes - cache->max_size);
    }

    return entry;
}

// 0x538D80
TigFileCacheItem* sub_538D80(TigFileCache* cache)
{
    return sub_538D90(cache);
}

// 0x538D90
TigFileCacheItem* sub_538D90(TigFileCache* cache)
{
    int attempt;
    int index;
    TigFileCacheItem* entry;

    for (attempt = 0; attempt < 4 * cache->capacity; attempt++) {
        index = rand() % cache->capacity;
        entry = &(cache->items[index]);
        if (entry->entry.data == NULL) {
            return entry;
        }

        if (entry->refcount == 0) {
            tig_file_cache_entry_remove(cache, entry);
            return entry;
        }
    }

    return sub_538E00(cache);
}

// 0x538E00
TigFileCacheItem* sub_538E00(TigFileCache* cache)
{
    int index;
    TigFileCacheItem* entry;

    for (index = 0; index < cache->capacity; index++) {
        entry = &(cache->items[index]);
        if (entry->entry.data == NULL) {
            return entry;
        }

        if (entry->refcount == 0) {
            tig_file_cache_entry_remove(cache, entry);
            return entry;
        }
    }

    return NULL;
}

// 0x538E40
void tig_file_cache_shrink(TigFileCache* cache, int size)
{
    int attempt;

    if (size > cache->bytes) {
        tig_file_cache_flush(cache);
    } else {
        tig_file_cache_removed_bytes = 0;

        for (attempt = 0; attempt < 4 * cache->capacity; attempt++) {
            sub_538D90(cache);
            if (tig_file_cache_removed_bytes >= size) {
                break;
            }
        }
    }
}

// 0x538E90
TigFileCacheEntry* tig_file_cache_acquire_internal(TigFileCache* cache, TigFileCacheItem* item)
{
    (void)cache;

    item->refcount++;
    return &(item->entry);
}

// 0x538EA0
void tig_file_cache_release(TigFileCache* cache, TigFileCacheEntry* entry)
{
    tig_file_cache_release_internal(cache, &(cache->items[entry->index]));
}

// 0x538EC0
void tig_file_cache_release_internal(TigFileCache* cache, TigFileCacheItem* item)
{
    (void)cache;

    time(&(item->timestamp));
    item->refcount--;
}
