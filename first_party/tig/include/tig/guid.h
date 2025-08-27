#ifndef TIG_GUID_H_
#define TIG_GUID_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TigGuid {
    uint8_t data[16];
} TigGuid;

// Serializeable.
static_assert(sizeof(TigGuid) == 16, "wrong size");

void tig_guid_create(TigGuid* guid);
bool tig_guid_is_equal(const TigGuid* a, const TigGuid* b);

#ifdef __cplusplus
}
#endif

#endif /* TIG_GUID_H_ */
