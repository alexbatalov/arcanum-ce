#ifndef ARCANUM_UI_WMAP_RND_H_
#define ARCANUM_UI_WMAP_RND_H_

#include "game/context.h"
#include "game/timeevent.h"

bool wmap_rnd_init(GameInitInfo* init_info);
void wmap_rnd_exit(void);
void wmap_rnd_reset(void);
bool wmap_rnd_mod_load(void);
void wmap_rnd_mod_unload(void);
bool wmap_rnd_save(TigFile* stream);
bool wmap_rnd_load(GameLoadInfo* load_info);
void wmap_rnd_disable(void);
bool wmap_rnd_timeevent_process(TimeEvent* timeevent);
void wmap_rnd_schedule(void);

#endif /* ARCANUM_UI_WMAP_RND_H_ */
