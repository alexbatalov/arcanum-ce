// TIMER
// ---
//
// The TIMER subsystem provides a couple of types and functions to deal with
// time points which usually represent engine or system-level events.
//
// This subsystem is not used for in-game events (instead it have it's own
// notion of game time, which is not a part of TIG).

#include "tig/timer.h"

#include <limits.h>

// 0x52DF80
int tig_timer_init(TigInitInfo* init_info)
{
    (void)init_info;

    return TIG_OK;
}

// 0x52DF90
void tig_timer_exit()
{
}

// 0x52DFA0
int tig_timer_now(tig_timestamp_t* timestamp_ptr)
{
    *timestamp_ptr = SDL_GetTicks() & UINT_MAX;
    return TIG_OK;
}

// 0x52DFB0
tig_duration_t tig_timer_elapsed(tig_timestamp_t start)
{
    return (SDL_GetTicks() & UINT_MAX) - start;
}

// 0x52DFC0
tig_duration_t tig_timer_between(tig_timestamp_t start, tig_timestamp_t end)
{
    unsigned int diff = end - start;
    if (diff > INT_MAX) {
        diff = INT_MAX;
    }
    return diff;
}
