#include "game/script_name.h"

#include <stdio.h>

/**
 * Index table for module-specific script files (1-999).
 *
 * 0x601750
 */
static TigIdxTable script_name_mod_idxtable;

/**
 * Index table for system script files (>=1000).
 *
 * 0x601760
 */
static TigIdxTable script_name_system_idxtable;

/**
 * Flag indicating whether the system script index table is loaded.
 *
 * 0x601770
 */
static bool script_name_system_loaded;

/**
 * Flag indicating whether the module-specific script index table is loaded.
 *
 * 0x601774
 */
static bool script_name_mod_loaded;

/**
 * Called when the game is initialized.
 *
 * 0x4CEC10
 */
bool script_name_init(GameInitInfo* init_info)
{
    TigFileList file_list;
    unsigned int index;
    int num;

    (void)init_info;

    // Initialize the system script index table with entry size suitable to hold
    // file path.
    tig_idxtable_init(&script_name_system_idxtable, TIG_MAX_PATH);

    tig_file_list_create(&file_list, "scr\\*.scr");

    // Iterate through all ".scr" files and associate system script number to
    // file path.
    for (index = 0; index < file_list.count; index++) {
        // NOTE: I believe there is a bug in separating system scripts from
        // module-specific scripts. Looking at the other features that have
        // system/module separation, this codepath should load generic scripts,
        // such as death, traps, items, credits, etc.
        //
        // Instead, this codepath loads all existing scripts except for:
        //  - 00997endslides.scr
        //  - 00998pcdeath.scr
        //  - 00999credits.scr
        //
        // Given the way the item prototype database is initialized, at least
        // all item scripts (including traps) must be categorized as system
        // scripts.
        num = atoi(file_list.entries[index].path);
        if (num >= 1000) {
            // Check for duplicate script number.
            if (tig_idxtable_contains(&script_name_system_idxtable, num)) {
                tig_debug_printf("Error! Multiple script files numbered %.5d\n", num);
            }

            tig_idxtable_set(&script_name_system_idxtable, num, file_list.entries[index].path);
        }
    }

    tig_file_list_destroy(&file_list);

    script_name_system_loaded = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4CECC0
 */
void script_name_exit(void)
{
    tig_idxtable_exit(&script_name_system_idxtable);
    script_name_system_loaded = false;
}

/**
 * Called when a module is being loaded.
 *
 * 0x4CECE0
 */
bool script_name_mod_load(void)
{
    TigFileList file_list;
    unsigned int index;
    int num;

    // Initialize the module-specific script index table with entry size
    // suitable to hold file path.
    tig_idxtable_init(&script_name_mod_idxtable, TIG_MAX_PATH);

    tig_file_list_create(&file_list, "scr\\*.scr");

    // Iterate through all ".scr" files and associate module-specific script
    // number to file path.
    for (index = 0; index < file_list.count; index++) {
        // NOTE: This codepath only loads the following scripts:
        //  - 00997endslides.scr
        //  - 00998pcdeath.scr
        //  - 00999credits.scr
        //
        // I believe virtually all character/tile/sector scripts should be here,
        // in the module-specific section.
        num = atoi(file_list.entries[index].path);
        if (num >= 1 && num < 1000) {
            // Check for duplicate script number.
            if (tig_idxtable_contains(&script_name_mod_idxtable, num)) {
                tig_debug_printf("Error! Multiple script files numbered %.5d\n", num);
            }
            tig_idxtable_set(&script_name_mod_idxtable, num, file_list.entries[index].path);
        }
    }

    tig_file_list_destroy(&file_list);

    script_name_mod_loaded = true;

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4CEDA0
 */
void script_name_mod_unload(void)
{
    tig_idxtable_exit(&script_name_mod_idxtable);
    script_name_mod_loaded = false;
}

/**
 * Builds a script file path from a given script num.
 *
 * 0x4CEDC0
 */
bool script_name_build_scr_name(int num, char* buffer)
{
    char path[TIG_MAX_PATH];

    if (num != 0) {
        if (script_name_system_loaded) {
            if (num >= 1000) {
                if (tig_idxtable_get(&script_name_system_idxtable, num, path)) {
                    sprintf(buffer, "scr\\%s", path);
                    return true;
                }
            }
        }

        if (script_name_mod_loaded) {
            if (num >= 1 && num < 1000) {
                if (tig_idxtable_get(&script_name_mod_idxtable, num, path)) {
                    sprintf(buffer, "scr\\%s", path);
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * Builds a dialogue file path from a given script num.
 *
 * 0x4CEE60
 */
bool script_name_build_dlg_name(int num, char* buffer)
{
    size_t len;

    // Build the script path.
    if (!script_name_build_scr_name(num, buffer)) {
        return false;
    }

    // Replace "scr" folder and "scr" extension with "dlg".
    len = strlen(buffer);
    buffer[0] = 'd';
    buffer[1] = 'l';
    buffer[2] = 'g';
    buffer[len - 3] = 'd';
    buffer[len - 2] = 'l';
    buffer[len - 1] = 'g';

    return true;
}
