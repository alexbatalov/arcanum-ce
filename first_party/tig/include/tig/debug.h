#ifndef TIG_DEBUG_H_
#define TIG_DEBUG_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int tig_debug_init(TigInitInfo* init_info);
void tig_debug_exit();
void tig_debug_printf(const char* format, ...);
void tig_debug_println(const char* string);

#ifdef __cplusplus
}
#endif

#endif /* TIG_DEBUG_H_ */
