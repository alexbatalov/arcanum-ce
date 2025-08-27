#include "tig/bsearch.h"

// NOTE: Original code is slightly different.
//
// 0x537940
void* tig_bsearch(void const* key, void const* base, size_t cnt, size_t size, int (*fn)(void const*, void const*), int* exists_ptr)
{
    unsigned char* l;
    unsigned char* r;
    unsigned char* mid;
    int index;
    int cmp;

    l = (unsigned char*)base;
    r = (unsigned char*)base + size * (cnt - 1);

    while (l <= r) {
        index = cnt / 2;
        if (index == 0) {
            if (cnt == 0) {
                *exists_ptr = 0;
                return l;
            }

            cmp = fn(key, l);
            if (cmp == 0) {
                *exists_ptr = 1;
                return l;
            }

            *exists_ptr = 0;
            return cmp < 0 ? l : l + size;
        }

        if ((cnt & 1) == 0) {
            index--;
        }

        mid = l + size * index;

        cmp = fn(key, mid);
        if (cmp == 0) {
            *exists_ptr = 1;
            return mid;
        }

        if (cmp < 0) {
            r = mid - size;
            if ((cnt & 1) != 0) {
                cnt /= 2;
            } else {
                cnt /= 2;
                cnt -= 1;
            }
        } else {
            l = mid + size;
            cnt /= 2;
        }
    }

    *exists_ptr = 0;
    return l;
}
