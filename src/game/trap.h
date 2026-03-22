#ifndef ARCANUM_GAME_TRAP_H_
#define ARCANUM_GAME_TRAP_H_

#include "game/context.h"
#include "game/object.h"
#include "game/script.h"
#include "game/timeevent.h"

typedef enum TrapType {
    TRAP_TYPE_INVALID,
    TRAP_TYPE_NONMAGICAL,
    TRAP_TYPE_MAGICAL,
} TrapType;

typedef enum TrapKnownReason {
    TRAP_KNOWN_SPOTTED,
    TRAP_KNOWN_PLACED,
} TrapKnownReason;

bool trap_init(GameInitInfo* init_info);
void trap_exit(void);
bool trap_timeevent_process(TimeEvent* timeevent);
int trap_type(int64_t obj);
bool trap_attempt_spot(int64_t pc_obj, int64_t trap_obj);
bool trap_is_spotted(int64_t pc_obj, int64_t trap_obj);
void trap_mark_known(int64_t pc_obj, int64_t trap_obj, int reason);
bool trap_use_on_obj(int64_t pc_obj, int64_t item_obj, int64_t target_obj);
bool trap_use_at_loc(int64_t pc_obj, int64_t item_obj, int64_t target_loc);
void trap_handle_disarm(int64_t pc_obj, int64_t trap_obj, bool* is_success_ptr, bool* is_critical_ptr);
bool trap_is_trap_device(int64_t obj);
int64_t get_trap_at_location(int64_t loc);
void trap_invoke(int64_t triggerer_obj, int64_t attachee_obj);
bool trap_script_execute(ScriptInvocation* invocation);

#endif /* ARCANUM_GAME_TRAP_H_ */
