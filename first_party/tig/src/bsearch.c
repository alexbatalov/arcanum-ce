#include "tig/bsearch.h"

// NOTE: Original code is slightly different.
//
// 0x537940
void* tig_bsearch(void const* key, void const* base, size_t cnt, size_t size, int (*fn)(void const*, void const*), int* exists_ptr)
{
    unsigned char* l = (unsigned char*)base;
    unsigned char* r = l + size * cnt;
    unsigned char* mid;
    int cmp;

    while (l < r) {
        mid = l + size * ((r - l) / size / 2);
        cmp = fn(key, mid);

        if (cmp == 0) {
            *exists_ptr = 1;
            return mid;
        } else if (cmp < 0) {
            r = mid;
        } else {
            l = mid + size;
        }
    }

    *exists_ptr = 0;
    return l;
}
