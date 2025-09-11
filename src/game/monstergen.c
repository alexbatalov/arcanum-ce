#include "game/monstergen.h"

#include "game/location.h"
#include "game/mp_utils.h"
#include "game/obj.h"
#include "game/random.h"
#include "game/target.h"
#include "game/tile.h"

#define DISABLED 0x80

static int monstergen_concurrent_get(int id);
static void monstergen_update(GeneratorInfo* info);
static int monstergen_concurrent_set(int id, int value);
static int monstergen_generate(GeneratorInfo* generator_info, int cnt);

/**
 * Defines time intervals (in-game time) for appropriate monster generator
 * rate.
 *
 * 0x5B5EC0
 */
static DateTime monstergen_time_intervals_tbl[GENERATOR_RATE_COUNT] = {
    /*      GENERATOR_RATE_SECOND */ { 0, 1000 },
    /* GENERATOR_RATE_HALF_MINUTE */ { 0, 30000 },
    /*      GENERATOR_RATE_MINUTE */ { 0, 60000 },
    /*        GENERATOR_RATE_HOUR */ { 0, 3600000 },
    /*         GENERATOR_RATE_DAY */ { 1, 0 },
    /*        GENERATOR_RATE_WEEK */ { 7, 0 },
    /*       GENERATOR_RATE_MONTH */ { 30, 0 },
    /*        GENERATOR_RATE_YEAR */ { 360, 0 },
};

/**
 * Array storing the shared state of each generator.
 *
 * Bits 0-4 specify the number of currently alive critters spawned by the
 * generator. Bit 7 is the disabled/enabled flag.
 *
 * 0x5FC350
 */
static uint8_t* monstergen_state;

/**
 * Parent window bounds.
 *
 * 0x5FC358
 */
static TigRect monstergen_iso_content_rect;

/**
 * Called when the game is initialized.
 *
 * 0x4BA380
 */
bool monstergen_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    monstergen_iso_content_rect.x = 0;
    monstergen_iso_content_rect.y = 0;
    monstergen_iso_content_rect.width = window_data.rect.width;
    monstergen_iso_content_rect.height = window_data.rect.height;

    monstergen_state = (uint8_t*)CALLOC(256, sizeof(*monstergen_state));

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4BA400
 */
void monstergen_reset()
{
    int index;

    for (index = 0; index < 256; index++) {
        monstergen_state[index] = 0;
    }
}

/**
 * Called when the game shuts down.
 *
 * 0x4BA420
 */
void monstergen_exit()
{
    FREE(monstergen_state);
}

/**
 * Called when the window size has changed.
 *
 * 0x4BA430
 */
void monstergen_resize(GameResizeInfo* resize_info)
{
    monstergen_iso_content_rect = resize_info->content_rect;
}

/**
 * Called when the game is being loaded.
 *
 * 0x4BA460
 */
bool monstergen_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(monstergen_state, 256, sizeof(*monstergen_state), load_info->stream) != 1) return false;

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x4BA490
 */
bool monstergen_save(TigFile* stream)
{
    if (tig_file_fwrite(monstergen_state, 256, sizeof(*monstergen_state), stream) != 1) return false;

    return true;
}

/**
 * Retrieves the configuration of a monster generator from a game object.
 *
 * 0x4BA500
 */
void monstergen_get(int64_t obj, GeneratorInfo* info)
{
    unsigned int data;

    data = obj_field_int32_get(obj, OBJ_F_NPC_GENERATOR_DATA);

    // Unpack generator data.
    info->flags = (data >> 27) & 0x1F;
    info->max_concurrent = (data >> 14) & 0x1F;
    info->obj = obj;
    info->id = (data >> 19);
    info->max_total = (data >> 7) & 0x7F;
    info->cur_total = data & 0x7F;
    info->cur_concurrent = monstergen_concurrent_get(info->id);
    info->rate = (obj_field_int32_get(obj, OBJ_F_NPC_FLAGS) >> 21) & (GENERATOR_RATE_COUNT - 1);
    info->enabled = !monstergen_is_disabled(info->id);
}

/**
 * Retrieves the number of alive monsters spawned by the given generator.
 *
 * 0x4BA590
 */
int monstergen_concurrent_get(int id)
{
    return id >= 0 && id < 256 ? monstergen_state[id] & 0x1F : 0;
}

/**
 * Configures a game object as a monster generator.
 *
 * 0x4BA5B0
 */
bool monstergen_set(GeneratorInfo* info)
{
    unsigned int npc_flags;

    // Only NPCs can be generators.
    if (obj_field_int32_get(info->obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return false;
    }

    // Mark as generator.
    npc_flags = obj_field_int32_get(info->obj, OBJ_F_NPC_FLAGS);
    npc_flags |= ONF_GENERATOR;
    mp_obj_field_int32_set(info->obj, OBJ_F_NPC_FLAGS, npc_flags);

    // Mark it invulnerable and disable rendering.
    mp_object_flags_set(info->obj, OF_INVULNERABLE | OF_OFF);

    // Update generator configuration.
    monstergen_update(info);

    return true;
}

/**
 * Synchronizes a generator's configuration with its game object and global
 * state.
 *
 * 0x4BA620
 */
void monstergen_update(GeneratorInfo* info)
{
    unsigned int data;
    unsigned int npc_flags;

    // Pack generator data.
    data = info->cur_total;
    data |= info->max_total << 7;
    data |= info->max_concurrent << 14;
    data |= info->id << 19;
    data |= info->flags << 27;
    obj_field_int32_set(info->obj, OBJ_F_NPC_GENERATOR_DATA, data);

    // Update the number of alive critters spawned by this generator.
    monstergen_concurrent_set(info->id, info->cur_concurrent);

    // Update spawn rate.
    npc_flags = obj_field_int32_get(info->obj, OBJ_F_NPC_FLAGS);
    npc_flags &= ~(ONF_GENERATOR_RATE1 | ONF_GENERATOR_RATE2 | ONF_GENERATOR_RATE3);
    npc_flags |= (info->rate & (GENERATOR_RATE_COUNT - 1)) << 21;
    obj_field_int32_set(info->obj, OBJ_F_NPC_FLAGS, npc_flags);

    // Toggle generator on/off.
    if (info->enabled) {
        monstergen_enable(info->id);
    } else {
        monstergen_disable(info->id);
    }
}

/**
 * Sets the number of alive critters spawned by the given generator.
 *
 * 0x4BA6D0
 */
int monstergen_concurrent_set(int id, int value)
{
    if (id < 0 || id >= 256) {
        return monstergen_concurrent_get(id);
    }

    if (value < 0 || value >= 32) {
        return monstergen_concurrent_get(id);
    }

    monstergen_state[id] &= ~0x1F;
    monstergen_state[id] |= value;

    return value;
}

/**
 * Reverts a game object from being a generator and converts it back to normal
 * NPC.
 *
 * 0x4BA720
 */
bool monstergen_remove(int64_t obj)
{
    unsigned int npc_flags;

    // Only NPCs can be generators.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return false;
    }

    // Check if the object is a generator.
    npc_flags = obj_field_int32_get(obj, OBJ_F_NPC_FLAGS);
    if ((npc_flags & ONF_GENERATOR) == 0) {
        return false;
    }

    // Unmark as being a generator.
    npc_flags &= ~ONF_GENERATOR;
    mp_obj_field_int32_set(obj, OBJ_F_NPC_FLAGS, npc_flags);

    // Restore vulnerability and enable rendering.
    mp_object_flags_unset(obj, OF_INVULNERABLE | OF_OFF);

    return true;
}

/**
 * Processes a generator.
 *
 * This function is the "hearbeat" function if the generator. It may or may not
 * spawn critters depending on configuration.
 *
 * Returns `true` if the generator is processed successfully, in this case
 * `datetime` indicates the next processing time. Returns `false` if the given
 * object is not a generator.
 *
 * 0x4BA790
 */
bool monstergen_process(int64_t obj, DateTime* datetime)
{
    GeneratorInfo generator_info;
    int num_monsters_created;
    int num_monsters_to_create;
    int remaining;
    TigRect rect;

    // Only NPCs can be generators.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return false;
    }

    // Check if the object is a generator.
    if ((obj_field_int32_get(obj, OBJ_F_NPC_FLAGS) & ONF_GENERATOR) == 0) {
        return false;
    }

    // Retrieve generator configuration.
    monstergen_get(obj, &generator_info);

    // Calculate the maximum number of monsters we should spawn in this cycle.
    num_monsters_to_create = generator_info.max_concurrent - generator_info.cur_concurrent;

    do {
        // Check if any monsters need to be spawned at all.
        if (num_monsters_to_create <= 0) {
            break;
        }

        // Check if the generator is disabled.
        if (monstergen_is_disabled(generator_info.id)) {
            break;
        }

        // Check time-of-day configuration.
        if (game_time_is_day()) {
            if ((generator_info.flags & GENERATOR_DAY) == 0) {
                break;
            }
        } else {
            if ((generator_info.flags & GENERATOR_NIGHT) == 0) {
                break;
            }
        }

        // Check if the generator is currently visible, but configured not to
        // spawn monsters while it is visible.
        //
        // NOTE: This condition might pose a problem on certain maps on very
        // high resolutions.
        if ((generator_info.flags & GENERATOR_INACTIVE_ON_SCREEN) != 0) {
            object_get_rect(obj, RECT_FLAG_IGNORE_VIS, &rect);
            if (rect.x < monstergen_iso_content_rect.width + monstergen_iso_content_rect.x
                && rect.y < monstergen_iso_content_rect.height + monstergen_iso_content_rect.y
                && monstergen_iso_content_rect.x < rect.x + rect.width
                && monstergen_iso_content_rect.y < rect.y + rect.height) {
                break;
            }
        }

        // Check total monster limit (unless it is configured to have no total
        // limit).
        if ((generator_info.flags & GENERATOR_IGNORE_TOTAL) == 0) {
            remaining = generator_info.max_total - generator_info.cur_total;
            if (remaining <= 0) {
                break;
            }

            // Make sure we won't spawn more than necessary.
            if (num_monsters_to_create > remaining) {
                num_monsters_to_create = remaining;
            }
        }

        // Ensure there are still monsters to spawn.
        if (num_monsters_to_create <= 0) {
            break;
        }

        // Spawn just one monster unless generator is configured to maintain
        // group of monsters at all times.
        if ((generator_info.flags & GENERATOR_SPAWN_ALL) == 0) {
            num_monsters_to_create = 1;
        }

        // Spawn the monsters.
        num_monsters_created = monstergen_generate(&generator_info, num_monsters_to_create);
        generator_info.cur_concurrent += num_monsters_created;

        // Update total count (unless it is configured to have no total limit,
        // in this case tracking totals is meaningless).
        if ((generator_info.flags & GENERATOR_IGNORE_TOTAL) == 0) {
            generator_info.cur_total += num_monsters_created;
        }

        // Update generator state.
        monstergen_update(&generator_info);
    } while (0);

    // Set the next processing time based on the generator's rate.
    *datetime = monstergen_time_intervals_tbl[generator_info.rate];

    return true;
}

/**
 * Spawns the specified number of monsters.
 *
 * Returns the number of monsters created.
 *
 * 0x4BA910
 */
int monstergen_generate(GeneratorInfo* generator_info, int cnt)
{
    int64_t loc;
    int64_t x;
    int64_t y;
    int64_t target_loc;
    int64_t target_x;
    int64_t target_y;
    tig_art_id_t art_id;
    int tile_type;
    int num_generated;
    int attempt;
    int distance;
    TigRect rect;
    int dx;
    int dy;
    int64_t obj;
    unsigned int npc_flags;

    // Retrieve monster generator data.
    loc = obj_field_int64_get(generator_info->obj, OBJ_F_LOCATION);
    art_id = tile_art_id_at(loc);
    tile_type = tig_art_tile_id_type_get(art_id);
    location_xy(loc, &x, &y);
    object_get_rect(generator_info->obj, RECT_FLAG_IGNORE_VIS, &rect);

    num_generated = 0;
    while (num_generated < cnt) {
        if (generator_info->max_concurrent == 1) {
            // Single monster always spawns at the generator's location.
            target_loc = loc;
        } else {
            // Try to find a suitable spawn location.
            for (attempt = 0; attempt < 10; attempt++) {
                // Pick random location within 5 tile radius and make sure it
                // has the same indoor/outdoor type. This way the engine tries
                // to guarantee that if generator is placed inside a structure,
                // all instances will also be spawned inside a structure.
                distance = random_between(1, 5);
                if (sub_4F4E40(generator_info->obj, distance, &target_loc)
                    && tile_type == tig_art_tile_id_type_get(tile_art_id_at(target_loc))) {
                    // Check on-screen configuration.
                    if ((generator_info->flags & GENERATOR_INACTIVE_ON_SCREEN) == 0) {
                        // No restrictions - spot found.
                        break;
                    }

                    // Check if the proposed location is not on the screen.
                    location_xy(target_loc, &target_x, &target_y);

                    dx = (int)(target_x - x + rect.x);
                    dy = (int)(target_y - y + rect.y);
                    if (dx >= monstergen_iso_content_rect.x + monstergen_iso_content_rect.width
                        || dy >= monstergen_iso_content_rect.y + monstergen_iso_content_rect.height
                        || monstergen_iso_content_rect.x >= dx + rect.width
                        || monstergen_iso_content_rect.y >= dy + rect.height) {
                        // Spot found.
                        break;
                    }
                }
            }

            // Fall back to the generator's location if no suitable spot is
            // found.
            if (attempt == 10) {
                target_loc = loc;
            }
        }

        // Create a deep copy (with inventory items) of the generator object.
        if (!mp_object_duplicate(generator_info->obj, target_loc, &obj)) {
            break;
        }

        // Unmark object as being a generator from the copied "instance".
        //
        // Note that it does not modify `OBJ_F_NPC_GENERATOR_DATA`, which stays
        // intact to maintain connection to generator object, and thus, access
        // shared generator state via `id`.
        monstergen_remove(obj);

        // Mark the "instance" as generated.
        npc_flags = obj_field_int32_get(obj, OBJ_F_NPC_FLAGS);
        npc_flags |= ONF_GENERATED;
        mp_obj_field_int32_set(obj, OBJ_F_NPC_FLAGS, npc_flags);

        num_generated++;
    }

    return num_generated;
}

/**
 * Called when the critter is killed.
 *
 * 0x4BAB30
 */
void monstergen_notify_killed(int64_t obj)
{
    unsigned int npc_flags;
    GeneratorInfo generator_info;

    // Ensure the object is a NPC.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return;
    }

    // Check if the object was generated
    npc_flags = obj_field_int32_get(obj, OBJ_F_NPC_FLAGS);
    if ((npc_flags & ONF_GENERATED) == 0) {
        return;
    }

    // Retrieve generator state and decrement number of alive critters spawned
    // by this generator.
    monstergen_get(obj, &generator_info);
    monstergen_concurrent_set(generator_info.id,
        monstergen_concurrent_get(generator_info.id) - 1);
}

/**
 * Checks if a generator is disabled.
 *
 * 0x4BABA0
 */
bool monstergen_is_disabled(int id)
{
    return id >= 0 && id < 256
        && (monstergen_state[id] & DISABLED) != 0;
}

/**
 * Enables a monster generator.
 *
 * 0x4BABD0
 */
void monstergen_enable(int id)
{
    if (id >= 0 && id < 256) {
        monstergen_state[id] &= ~DISABLED;
    }
}

/**
 * Disables a monster generator.
 *
 * 0x4BABF0
 */
void monstergen_disable(int id)
{
    if (id >= 0 && id < 256) {
        monstergen_state[id] |= DISABLED;
    }
}
