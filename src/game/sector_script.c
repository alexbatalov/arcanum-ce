#include "game/sector_script.h"

#include "game/sector.h"

/**
 * Called when the game is initialized.
 *
 * 0x4C0410
 */
bool sector_script_init(GameInitInfo* init_info)
{
    (void)init_info;

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4C0420
 */
void sector_script_reset(void)
{
}

/**
 * Called when the game shuts down.
 *
 * 0x4C0430
 */
void sector_script_exit(void)
{
}

/**
 * Retrieves the script associated with a sector.
 *
 * 0x4C0440
 */
bool sector_script_get(int64_t sector_id, Script* scr)
{
    Sector* sector;
    bool ret = false;

    if (sector_lock(sector_id, &sector)) {
        ret = sector_script_list_get(&(sector->sector_scripts), scr);
        sector_unlock(sector_id);
    }

    return ret;
}

/**
 * Sets the script for a sector.
 *
 * 0x4C0490
 */
bool sector_script_set(int64_t sector_id, Script* scr)
{
    Sector* sector;
    bool ret = false;

    if (sector_lock(sector_id, &sector)) {
        ret = sector_script_list_set(&(sector->sector_scripts), scr);
        sector_unlock(sector_id);
    }

    return ret;
}
