#ifndef TIG_BSEARCH_H_
#define TIG_BSEARCH_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* tig_bsearch(void const* key, void const* base, size_t cnt, size_t size, int (*fn)(void const*, void const*), int* exists_ptr);

#ifdef __cplusplus
}
#endif

#endif /* TIG_BSEARCH_H_ */
