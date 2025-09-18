#ifndef TIG_KB_H_
#define TIG_KB_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int tig_kb_init(TigInitInfo* init_info);
void tig_kb_exit();
bool tig_kb_is_key_pressed(SDL_Scancode scancode);
bool tig_kb_get_modifier(SDL_Keymod keymod);
void tig_kb_set_key(SDL_Keycode key, SDL_Scancode scancode, bool down);

#ifdef __cplusplus
}
#endif

#endif /* TIG_KB_H_ */
