#include "game/portrait.h"

#include <stdio.h>

#include "game/mes.h"
#include "game/multiplayer.h"
#include "game/player.h"
#include "game/stat.h"

static void portrait_draw_func(int64_t obj, int num, tig_window_handle_t window_handle, int x, int y, int width, int height);
static bool portrait_is_valid(int64_t obj, const char* str);

/**
 * 0x5B7C0C
 */
static const char* portrait_race_specifiers[RACE_COUNT] = {
    /*     RACE_HUMAN */ "HU",
    /*     RACE_DWARF */ "DW",
    /*       RACE_ELF */ "EL",
    /*  RACE_HALF_ELF */ "HE",
    /*     RACE_GNOME */ "GN",
    /*  RACE_HALFLING */ "HA",
    /*  RACE_HALF_ORC */ "HO",
    /* RACE_HALF_OGRE */ "HG",
    /*  RACE_DARK_ELF */ "EL",
    /*      RACE_OGRE */ "HG",
    /*       RACE_ORC */ "HO",
};

/**
 * 0x5B7C38
 */
static char portrait_gender_specifiers[GENDER_COUNT] = {
    /* GENDER_FEMALE */ 'F',
    /*   GENDER_MALE */ 'M',
};

/**
 * "userport.mes"
 *
 * 0x601744
 */
static mes_file_handle_t portrait_user_mes_file;

/**
 * Flag indicating if the user-specific portrait file is loaded.
 *
 * 0x601748
 */
static bool portrait_have_user_mes_file;

/**
 * "gameport.mes"
 *
 * 0x60174C
 */
static mes_file_handle_t portrait_mes_file;

/**
 * Called when the game is initialized.
 *
 * 0x4CE350
 */
bool portrait_init(GameInitInfo* init_info)
{
    (void)init_info;

    // Load game portraits message file (required).
    if (!mes_load("portrait\\gameport.mes", &portrait_mes_file)) {
        return false;
    }

    // Load user portraits message file (optional).
    portrait_have_user_mes_file = mes_load("portrait\\userport.mes", &portrait_user_mes_file);

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4CE390
 */
void portrait_exit(void)
{
    mes_unload(portrait_mes_file);

    if (portrait_have_user_mes_file) {
        mes_unload(portrait_user_mes_file);
    }
}

/**
 * Constructs the path to a portrait .BMP file for a portrait based on it's
 * number and given size class.
 *
 * 0x4CE3C0
 */
void portrait_path(int num, char* path, int size)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    // Determine message file to use based on portrait ID (separating system
    // vs. user-specific portraits).
    mes_file_entry.num = num;
    if (num >= 1000) {
        // Use engine-provided portraits.
        mes_file = portrait_mes_file;
    } else if (num >= 1) {
        // Use user-specific portraits.
        if (!portrait_have_user_mes_file) {
            // User portrait was requested, but none are present.
            path[0] = '\0';
            return;
        }

        mes_file = portrait_user_mes_file;
    } else {
        // Default portrait.
        mes_file = portrait_mes_file;
        mes_file_entry.num = 1000;
    }

    // Search for the portrait entry.
    if (!mes_search(mes_file, &mes_file_entry)) {
        path[0] = '\0';
        return;
    }

    // Construct file path based on size class.
    if (size >= 128) {
        sprintf(path, "portrait\\%s_b.bmp", mes_file_entry.str);
    } else {
        sprintf(path, "portrait\\%s.bmp", mes_file_entry.str);
    }
}

/**
 * Draws a portrait using its native size.
 *
 * 0x4CE470
 */
void portrait_draw_native(int64_t obj, int num, tig_window_handle_t window_handle, int x, int y)
{
    portrait_draw_func(obj, num, window_handle, x, y, 0, 0);
}

/**
 * CE: The original code uses TIG BMP API. This implementation prefers to load
 * portrait directly into the video buffer.
 *
 * 0x4CE4A0
 */
void portrait_draw_func(int64_t obj, int num, tig_window_handle_t window_handle, int x, int y, int width, int height)
{
    char path[TIG_MAX_PATH];
    TigVideoBuffer* video_buffer;
    TigVideoBufferData video_buffer_data;
    int rc;
    char* pch;
    TigRect src_rect;
    TigRect dst_rect;
    TigWindowBlitInfo blit_info;

    // Check if the object is a player character with alternate data.
    if (obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && (obj_field_int32_get(obj, OBJ_F_PC_FLAGS) & OPCF_USE_ALT_DATA) != 0) {
        // Attempt to construct path for a multiplayer-specific portrait.
        if (!multiplayer_portrait_path(obj, width, path)) {
            return;
        }
    } else {
        // Build single-player portrait path.
        portrait_path(num, path, width);
    }

    // Load bitmap from the portrait path.
    rc = tig_video_buffer_load_from_bmp(path, &video_buffer, 0x01);
    if (rc != TIG_OK) {
        // Something wrong was with the portrait. Check if a big portrait was
        // requested and if so, fallback to normal size portrait.
        if (strlen(path) >= 6) {
            pch = &(path[strlen(path) - 6]);
            if (SDL_strcasecmp(pch, "_b.bmp") == 0) {
                *pch = '\0';
                strcat(path, ".bmp");
                rc = tig_video_buffer_load_from_bmp(path, &video_buffer, 0x01);
            }
        }
    }

    if (rc != TIG_OK) {
        return;
    }

    if (tig_video_buffer_data(video_buffer, &video_buffer_data) != TIG_OK) {
        tig_video_buffer_destroy(video_buffer);
        return;
    }

    // Set up source rect to the full dimensions.
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = video_buffer_data.width;
    src_rect.height = video_buffer_data.height;

    // Set up destination rect based on the provided constraints (applies
    // scaling).
    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.width = video_buffer_data.width;
    dst_rect.height = video_buffer_data.height;

    if (width > 0) {
        dst_rect.width = width;
    }

    if (height > 0) {
        dst_rect.height = height;
    }

    blit_info.type = TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW;
    blit_info.vb_blit_flags = 0;
    blit_info.src_rect = &src_rect;
    blit_info.src_video_buffer = video_buffer;
    blit_info.dst_rect = &dst_rect;
    blit_info.dst_window_handle = window_handle;
    tig_window_blit(&blit_info);
    tig_video_buffer_destroy(video_buffer);
}

/**
 * Draws the specified portrait scaled to 128x128 pixels.
 *
 * 0x4CE640
 */
void portrait_draw_128x128(int64_t obj, int num, tig_window_handle_t window_handle, int x, int y)
{
    portrait_draw_func(obj, num, window_handle, x, y, 128, 128);
}

/**
 * Draws the specified portrait scaled to 32x32 pixels.
 *
 * 0x4CE680
 */
void portrait_draw_32x32(int64_t obj, int num, tig_window_handle_t window_handle, int x, int y)
{
    portrait_draw_func(obj, num, window_handle, x, y, 32, 32);
}

/**
 * Finds the first portrait suitable for the specified game object.
 *
 * 0x4CE6B0
 */
bool portrait_find_first(int64_t obj, int* portrait_ptr)
{
    MesFileEntry mes_file_entry;

    *portrait_ptr = 0;

    // Skip if the object is a player character with alternate data
    // (used in multiplayer).
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && (obj_field_int32_get(obj, OBJ_F_PC_FLAGS) & OPCF_USE_ALT_DATA) != 0) {
        return false;
    }

    // First, search engine portraits (starting from ID 1000).
    mes_file_entry.num = 1000;
    while (mes_search(portrait_mes_file, &mes_file_entry)) {
        if (portrait_is_valid(obj, mes_file_entry.str)) {
            *portrait_ptr = mes_file_entry.num;
            return true;
        }
        mes_file_entry.num++;
    }

    // If there are no engine portraits, try user portraits (range 1-999).
    if (portrait_have_user_mes_file) {
        mes_file_entry.num = 1;
        while (mes_file_entry.num < 1000 && mes_search(portrait_user_mes_file, &mes_file_entry)) {
            if (portrait_is_valid(obj, mes_file_entry.str)) {
                *portrait_ptr = mes_file_entry.num;
                return true;
            }
            mes_file_entry.num++;
        }
    }

    return false;
}

/**
 * Finds the last portrait suitable for the specified game object.
 *
 * 0x4CE7A0
 */
bool portrait_find_last(int64_t obj, int* portrait_ptr)
{
    MesFileEntry mes_file_entry;
    int num = 0;

    // Skip if the object is a player character with alternate data
    // (used in multiplayer).
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && (obj_field_int32_get(obj, OBJ_F_PC_FLAGS) & OPCF_USE_ALT_DATA) != 0) {
        return false;
    }

    // Search engine portraits starting from the provided ID.
    mes_file_entry.num = *portrait_ptr;
    while (mes_search(portrait_mes_file, &mes_file_entry)) {
        if (portrait_is_valid(obj, mes_file_entry.str)) {
            num = mes_file_entry.num;
        }
        mes_file_entry.num++;
    }

    if (num != 0) {
        *portrait_ptr = num;
        return true;
    }

    // Next up, try user portraits.
    if (portrait_have_user_mes_file) {
        mes_file_entry.num = 1;
        while (mes_file_entry.num < 1000 && mes_search(portrait_user_mes_file, &mes_file_entry)) {
            if (portrait_is_valid(obj, mes_file_entry.str)) {
                num = mes_file_entry.num;
            }
            mes_file_entry.num++;
        }
    }

    if (num != 0) {
        *portrait_ptr = num;
        return true;
    }

    return false;
}

/**
 * Finds next valid portrait for the specified game object.
 *
 * 0x4CE8B0
 */
bool portrait_find_next(int64_t obj, int* portrait_ptr)
{
    MesFileEntry mes_file_entry;

    // Skip if the object is a player character with alternate data.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && (obj_field_int32_get(obj, OBJ_F_PC_FLAGS) & OPCF_USE_ALT_DATA) != 0) {
        return false;
    }

    // Start searching from the next ID.
    mes_file_entry.num = *portrait_ptr + 1;

    if (mes_file_entry.num >= 1000) {
        // Continue search the engine-provided portraits.
        while (mes_search(portrait_mes_file, &mes_file_entry)) {
            if (portrait_is_valid(obj, mes_file_entry.str)) {
                *portrait_ptr = mes_file_entry.num;
                return true;
            }
            mes_file_entry.num++;
        }

        // No engine-provided portrait found, attempt to continue with user
        // portraits.
        mes_file_entry.num = 1;
    }

    // Continue with user portraits.
    if (portrait_have_user_mes_file) {
        while (mes_file_entry.num < 1000 && mes_search(portrait_user_mes_file, &mes_file_entry)) {
            if (portrait_is_valid(obj, mes_file_entry.str)) {
                *portrait_ptr = mes_file_entry.num;
                return true;
            }
            mes_file_entry.num++;
        }
    }

    return false;
}

/**
 * Finds previous valid portrait for the specified game object.
 *
 * 0x4CE9B0
 */
bool portrait_find_prev(int64_t obj, int* portrait_ptr)
{
    MesFileEntry mes_file_entry;

    // Skip if the object is a player character with alternate data.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && (obj_field_int32_get(obj, OBJ_F_PC_FLAGS) & OPCF_USE_ALT_DATA) != 0) {
        return false;
    }

    // Exit if already at the default portrait ID.
    if (*portrait_ptr == 1000) {
        return false;
    }

    // Start searching from the previous ID.
    mes_file_entry.num = *portrait_ptr - 1;

    if (mes_file_entry.num < 1000) {
        // Search user portraits.
        if (portrait_have_user_mes_file) {
            while (mes_file_entry.num >= 1 && mes_search(portrait_user_mes_file, &mes_file_entry)) {
                if (portrait_is_valid(obj, mes_file_entry.str)) {
                    *portrait_ptr = mes_file_entry.num;
                    return true;
                }
                mes_file_entry.num--;
            }
        }

        // No user portraits found, move cursor to the last engine-provided
        // portrait.
        mes_file_entry.num = mes_entries_count(portrait_mes_file) + 999;
    }

    // Search engine-provided portraits.
    while (mes_file_entry.num >= 1000 && mes_search(portrait_mes_file, &mes_file_entry)) {
        if (portrait_is_valid(obj, mes_file_entry.str)) {
            *portrait_ptr = mes_file_entry.num;
            return true;
        }
        mes_file_entry.num--;
    }

    return false;
}

/**
 * Checks if a portrait prefix matches critter's race and gender.
 *
 * 0x4CEAC0
 */
bool portrait_is_valid(int64_t obj, const char* str)
{
    int race;
    int gender;
    char buffer[4];

    if (str != NULL) {
        race = stat_base_get(obj, STAT_RACE);
        gender = stat_base_get(obj, STAT_GENDER);

        // Construct the exact portrait prefix.
        sprintf(buffer, "%s%c",
            portrait_race_specifiers[race],
            portrait_gender_specifiers[gender]);

        // Check if portrait file name starts with the race-gender specifiers.
        if (SDL_strncasecmp(buffer, str, 3) == 0) {
            return true;
        }

        // Special case - portraits prefixed with "ANY" are suitable for any
        // critter.
        if (SDL_strncasecmp("ANY", str, 3) == 0) {
            return true;
        }

        // Special case - portraits prefixed with "NPC" are suitable for NPCs.
        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC
            && SDL_strncasecmp("NPC", str, 3) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Retrieves the portrait of a critter.
 *
 * 0x4CEB80
 */
int portrait_get(int64_t obj)
{
    int type;
    int portrait;
    int next;
    int prev;

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    portrait = obj_field_int32_get(obj, OBJ_F_CRITTER_PORTRAIT);

    // Special case - the player is the *Living One*, no one dare to wear his
    // appearance!
    if (portrait != 0
        && type == OBJ_TYPE_NPC
        && portrait == portrait_get(player_get_local_pc_obj())) {
        // Fallback to next/prev portrait.
        if (portrait_find_next(obj, &next)) {
            portrait = next;
        } else if (portrait_find_prev(obj, &prev)) {
            portrait = prev;
        }
    }

    return portrait;
}
