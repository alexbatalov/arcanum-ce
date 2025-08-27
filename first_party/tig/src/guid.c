#include "tig/guid.h"

#include <stdlib.h>

/**
 * Generates UUID v4.
 *
 * This function is a replacement for `CoCreateGuid`, but it uses pseudo-random
 * numbers as a source.
 */
void tig_guid_create(TigGuid* guid)
{
    uint8_t bytes[16];
    int i;

    for (i = 0; i < 16; i++) {
        bytes[i] = rand() % 256;
    }

    // http://tools.ietf.org/html/rfc4122.html#section-4.4
    bytes[6] = (bytes[6] & 0x0F) | 0x40; // Version 4
    bytes[8] = (bytes[8] & 0x3F) | 0x80; // Variant 1

    memcpy(guid, bytes, sizeof(bytes));
}

bool tig_guid_is_equal(const TigGuid* a, const TigGuid* b)
{
    return memcmp(a, b, sizeof(TigGuid)) == 0;
}
