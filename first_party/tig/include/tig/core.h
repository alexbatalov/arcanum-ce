#ifndef TIG_CORE_H_
#define TIG_CORE_H_

#include "tig/timer.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern tig_timestamp_t tig_ping_timestamp;

int tig_init(TigInitInfo* init_info);
void tig_exit(void);
void tig_ping();
void sub_51F250();
void tig_set_active(bool is_active);
bool tig_get_active();

#ifdef __cplusplus
}
#endif

#endif /* TIG_CORE_H_ */
