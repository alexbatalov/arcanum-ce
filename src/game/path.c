#include "game/path.h"

#include "game/critter.h"
#include "game/location.h"
#include "game/object.h"
#include "game/sector.h"
#include "game/terrain.h"
#include "game/tile.h"
#include "game/townmap.h"
#include "game/trap.h"

typedef struct S420330 {
    /* 0000 */ int field_0;
    /* 0004 */ int field_4;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
    /* 0010 */ int64_t field_10;
    /* 0018 */ int64_t field_18;
    /* 0020 */ int8_t* rotations;
} S420330;

static int sub_41F6C0(PathCreateInfo* path_create_info);
static int sub_41F840(PathCreateInfo* path_create_info);
static int sub_41F9F0(PathCreateInfo* path_create_info);
static int path_dist(int src, int dst, int width);
static int sub_420110(int a1, int a2, int a3);
static void sub_420330(int64_t x, int64_t y, S420330* a5);
static void sub_4203B0(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, S420330* a5, void (*fn)(int64_t, int64_t, S420330*));
static int sub_420660(int64_t from, int64_t to, int8_t* rotations);
static int sub_420900(WmapPathInfo* path_info);
static int sub_4209C0(WmapPathInfo* path_info);
static int sub_420E30(PathCreateInfo* path_create_info, tig_duration_t ms);

// 0x5A15C0
static int path_limit = 10;

// 0x5A15C4
static int path_time_limit = 250;

// 0x5D5628
static int path_cost_tbl[4096];

// 0x5D9628
static int path_backtrack_tbl[4096];

// NOTE: Unusual size.
//
// 0x5DD628
static int wmap_path_backtrack_tbl[756];

// 0x5DE1F8
static int wmap_path_cost_tbl[256];

// 0x5DE5F8
static tig_timestamp_t dword_5DE5F8;

// 0x5DE5FC
static int dword_5DE5FC;

// 0x5DE600
static tig_duration_t dword_5DE600;

// 0x41F3C0
int path_make(PathCreateInfo* path_create_info)
{
    bool v1 = false;

    if ((path_create_info->flags & PATH_FLAG_0x0800) != 0) {
        return sub_41F6C0(path_create_info);
    }

    path_create_info->field_24 = 0;

    if (path_create_info->from == path_create_info->to) {
        return 0;
    }

    if ((path_create_info->flags & PATH_FLAG_0x0008) != 0) {
        v1 = true;
    }

    if ((path_create_info->flags & PATH_FLAG_0x0080) != 0) {
        return sub_420660(path_create_info->from, path_create_info->to, path_create_info->rotations);
    }

    if ((path_create_info->flags & PATH_FLAG_0x0100) != 0) {
        return sub_4201C0(path_create_info->from, path_create_info->to, path_create_info->rotations);
    }

    if ((path_create_info->flags & (PATH_FLAG_0x0040 | PATH_FLAG_0x0001)) == 0) {
        ObjectList objects;
        ObjectNode* node;

        if ((path_create_info->flags & PATH_FLAG_0x0008) == 0
            && tile_is_blocking(path_create_info->to, v1)) {
            return 0;
        }

        object_list_location(path_create_info->to, OBJ_TM_CRITTER | OBJ_TM_SCENERY | OBJ_TM_CONTAINER, &objects);
        node = objects.head;
        while (node != NULL) {
            if ((obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_NO_BLOCK) == 0) {
                if (!obj_type_is_critter(obj_field_int32_get(node->obj, OBJ_F_TYPE))
                    || !critter_is_dead(node->obj)) {
                    break;
                }
            }
            node = node->next;
        }
        object_list_destroy(&objects);

        if (node != NULL) {
            return false;
        }
    }

    int v2 = 0;
    if ((path_create_info->flags & PATH_FLAG_0x0200) == 0) {
        v2 = sub_41F840(path_create_info);
    }

    if (v2 == 0) {
        if ((path_create_info->flags & PATH_FLAG_0x1000) != 0) {
            v2 = sub_420E30(path_create_info, 300);
        } else {
            if ((path_create_info->flags & PATH_FLAG_0x0020) != 0) {
                return 0;
            }

            v2 = sub_41F9F0(path_create_info);
        }
    }

    if (v2 > 8) {
        v2 = 8;
    }

    return v2;
}

// 0x41F570
unsigned int path_translate_flags(PathFlags flags)
{
    unsigned int new_flags = 0;

    if ((flags & PATH_FLAG_0x0002) != 0) {
        new_flags |= OBJ_TRAVERSAL_PORTAL_BLOCKS;
    }

    if ((flags & PATH_FLAG_0x0004) != 0) {
        new_flags |= OBJ_TRAVERSAL_WINDOW_BLOCKS;
    }

    if ((flags & PATH_FLAG_0x0010) != 0) {
        new_flags |= OBJ_TRAVERSAL_IGNORE_CRITTERS;
    }

    return new_flags;
}

// 0x41F6C0
int sub_41F6C0(PathCreateInfo* path_create_info)
{
    int64_t dist;
    int rotations[8];
    int range;
    int base_rot;
    int idx;
    int v1;
    int v2;
    int64_t loc;

    rotations[0] = 4;
    rotations[1] = 5;
    rotations[2] = 3;
    rotations[3] = 6;
    rotations[4] = 2;
    rotations[5] = 7;
    rotations[6] = 1;
    rotations[7] = 0;

    path_create_info->flags &= ~PATH_FLAG_0x0800;

    dist = location_dist(path_create_info->from, path_create_info->to);

    if (dist >= INT_MAX) {
        return 0;
    }

    range = path_create_info->field_24 - (int)dist;
    base_rot = location_rot(path_create_info->from, path_create_info->to);

    v2 = 0;
    for (idx = 0; idx < 8; idx++) {
        location_in_range(path_create_info->from,
            (base_rot + rotations[idx]) % 8,
            range,
            &(path_create_info->to));
        v1 = sub_41F840(path_create_info);
        if (v1 != 0) {
            break;
        }

        if (v2 < path_create_info->field_24) {
            v2 = path_create_info->field_24;
            loc = path_create_info->to;
        }
    }

    path_create_info->flags |= PATH_FLAG_0x0800;

    if (v1 == 0 && v2 > 0) {
        path_create_info->to = loc;
        v1 = sub_41F840(path_create_info);
    }

    return v1;
}

// 0x41F840
int sub_41F840(PathCreateInfo* path_create_info)
{
    bool v1 = false;
    unsigned int flags;
    int idx;
    int64_t loc;
    int rot;
    int64_t adjacent_loc;
    bool is_window;
    int64_t trap_obj;

    if (location_dist(path_create_info->from, path_create_info->to) > 32) {
        return 0;
    }

    if ((path_create_info->flags & PATH_FLAG_0x0008) != 0) {
        v1 = true;
    }

    flags = path_translate_flags(path_create_info->flags);

    loc = path_create_info->from;
    for (idx = 0; idx < path_create_info->max_rotations; idx++) {
        if (loc == path_create_info->to) {
            break;
        }

        rot = location_rot(loc, path_create_info->to);
        path_create_info->rotations[idx] = (int8_t)rot;
        location_in_dir(loc, rot, &adjacent_loc);

        if ((path_create_info->flags & PATH_FLAG_0x0040) == 0) {
            if (adjacent_loc != path_create_info->to
                || (path_create_info->flags & PATH_FLAG_0x0001) == 0) {
                if ((path_create_info->flags & PATH_FLAG_0x0008) == 0
                    && tile_is_blocking(adjacent_loc, v1)) {
                    break;
                }

                if (object_traversal_is_blocked(path_create_info->obj, loc, rot, flags, &is_window)) {
                    break;
                }

                if (is_window) {
                    break;
                }

                trap_obj = get_trap_at_location(adjacent_loc);
                if (trap_obj != OBJ_HANDLE_NULL
                    && trap_is_spotted(path_create_info->obj, trap_obj)) {
                    break;
                }

                if ((path_create_info->flags & PATH_FLAG_0x0400) != 0
                    && get_fire_at_location(adjacent_loc) != OBJ_HANDLE_NULL) {
                    break;
                }
            }
        }

        loc = adjacent_loc;
    }

    if (loc != path_create_info->to) {
        path_create_info->field_24 = idx;
        return 0;
    }

    if ((path_create_info->flags & PATH_FLAG_0x0001) != 0) {
        idx--;
    }

    return idx;
}

// 0x41F9F0
int sub_41F9F0(PathCreateInfo* path_create_info)
{
    tig_timestamp_t timestamp;
    bool v1 = false;
    unsigned int flags;
    int64_t from_x;
    int64_t from_y;
    int64_t to_x;
    int64_t to_y;
    int64_t origin_x;
    int64_t origin_y;
    int64_t dx;
    int64_t dy;
    int start_index;
    int target_index;
    int current_index;
    int best_estimated_cost;

    if (obj_field_int32_get(path_create_info->obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        if (dword_5DE5F8 == 0
            || tig_timer_elapsed(dword_5DE5F8) >= 1000) {
            tig_timer_now(&dword_5DE5F8);
            dword_5DE5FC = 0;
            dword_5DE600 = 0;
        }

        if (dword_5DE5FC > path_limit) {
            return 0;
        }

        if (dword_5DE600 > path_time_limit) {
            return 0;
        }

        dword_5DE5FC++;

        tig_timer_now(&timestamp);
    } else {
        timestamp = 0;
    }

    from_x = LOCATION_GET_X(path_create_info->from);
    from_y = LOCATION_GET_Y(path_create_info->from);
    to_x = LOCATION_GET_X(path_create_info->to);
    to_y = LOCATION_GET_Y(path_create_info->to);

    if ((path_create_info->flags & PATH_FLAG_0x0008) != 0) {
        v1 = true;
    }

    flags = path_translate_flags(path_create_info->flags);

    if (from_x > to_x) {
        dx = from_x - to_x;
        origin_x = to_x;
    } else {
        dx = to_x - from_x;
        origin_x = from_x;
    }

    if (from_y > to_y) {
        dy = from_y - to_y;
        origin_y = to_y;
    } else {
        dy = to_y - from_y;
        origin_y = from_y;
    }

    // Check if the locations are too far apart.
    if (dx > 32 || dy > 32) {
        return 0;
    }

    origin_x -= (64 - dx) / 2;
    origin_y -= (64 - dy) / 2;

    start_index = (int)(from_x + (from_y - origin_y) * 64 - origin_x);
    target_index = (int)(to_x + (to_y - origin_y) * 64 - origin_x);

    // Initialize the cost array to zero (unprocessed state).
    memset(path_cost_tbl, 0, sizeof(path_cost_tbl));
    path_cost_tbl[start_index] = 1;
    path_backtrack_tbl[start_index] = -1;

    while (true) {
        current_index = -1;

        // Grab open node with minimal cost.
        for (int i = 0; i < 4096; i++) {
            // Check if node is open, i.e. it has a positive cost (negative cost
            // are closed nodes, zero cost are unprocessed nodes).
            if (path_cost_tbl[i] > 0) {
                int estimated_cost = path_cost_tbl[i] + path_dist(i, target_index, 64);
                if (estimated_cost / 10 <= path_create_info->max_rotations) {
                    // If no candidate node selected yet, or this candidate has
                    // a lower cost, update current candidate.
                    if (current_index == -1 || estimated_cost < best_estimated_cost) {
                        best_estimated_cost = estimated_cost;
                        current_index = i;
                    }
                } else {
                    // Mark node as unreachable.
                    path_cost_tbl[i] = -32768;
                }
            }
        }

        // If no node has been found, path is not reachable.
        if (current_index == -1) {
            if (timestamp != 0) {
                dword_5DE600 += tig_timer_elapsed(timestamp);
            }

            return 0;
        }

        // Check if we have reached the target.
        if (current_index == target_index) {
            break;
        }

        for (int rot = 0; rot < 8; rot++) {
            int64_t loc = location_make(origin_x + current_index % 64, origin_y + current_index / 64);
            int64_t adjacent_loc;
            if (!location_in_dir(loc, rot, &adjacent_loc)) {
                continue;
            }

            dx = LOCATION_GET_X(adjacent_loc) - origin_x;
            dy = LOCATION_GET_Y(adjacent_loc) - origin_y;
            if (dx < 0 || dx >= 64 || dy < 0 || dy >= 64) {
                continue;
            }

            int neighbor_index = (int)dx + (int)dy * 64;

            // Skip if the neighbor has already been marked as unreachable.
            if (path_cost_tbl[neighbor_index] == -32768) {
                continue;
            }

            bool is_window = false;
            if ((path_create_info->flags & PATH_FLAG_0x0040) == 0) {
                if (adjacent_loc != path_create_info->to
                    || (path_create_info->flags & PATH_FLAG_0x0001) == 0) {
                    if ((path_create_info->flags & PATH_FLAG_0x0008) == 0
                        && tile_is_blocking(adjacent_loc, v1)) {
                        path_cost_tbl[neighbor_index] = -32768;
                        continue;
                    }

                    int64_t block_obj;
                    int block_obj_type;
                    if (object_calc_traversal_cost(path_create_info->obj, loc, rot, flags, &block_obj, &block_obj_type, &is_window)
                        || block_obj != OBJ_HANDLE_NULL) {
                        if ((rot & 1) != 0
                            && block_obj != OBJ_HANDLE_NULL
                            && block_obj_type != OBJ_TYPE_WALL
                            && block_obj_type != OBJ_TYPE_PORTAL) {
                            path_cost_tbl[neighbor_index] = -32768;
                        }
                        continue;
                    }
                }
            }

            // Base movement cost.
            int cost = 10;

            // Evade spotted traps.
            int64_t trap_obj = get_trap_at_location(adjacent_loc);
            if (trap_obj != OBJ_HANDLE_NULL) {
                if (trap_is_spotted(path_create_info->obj, trap_obj)) {
                    cost += 80;
                }
            }

            if (is_window) {
                cost += 50;
            }

            // Evade fire.
            if ((path_create_info->flags & PATH_FLAG_0x0400) != 0
                && get_fire_at_location(adjacent_loc) != OBJ_HANDLE_NULL) {
                cost += 80;
            }

            // Evade differently lit areas.
            if ((path_create_info->flags & PATH_FLAG_0x0200) != 0) {
                uint8_t src_lum = sub_4D9240(loc, 0, 0);
                uint8_t dst_lum = sub_4D9240(adjacent_loc, 0, 0);
                if (dst_lum > src_lum) {
                    if (dst_lum - src_lum > 20) {
                        cost += 40;
                    }
                } else {
                    if (src_lum - dst_lum > 20) {
                        cost -= 6;
                    }
                }
            }

            // Add accumulated cost to move the current node.
            cost += path_cost_tbl[current_index];

            // Add additional cost in case direction change is needed.
            if (path_backtrack_tbl[current_index] != -1
                && sub_420110(path_backtrack_tbl[current_index], current_index, 64) != rot) {
                cost++;
            }

            // If this neighbor has not been reached or a lower cost is now
            // available, update its cost and record the current node as its
            // predecessor.
            if ((path_cost_tbl[neighbor_index] > 0 && path_cost_tbl[neighbor_index] > cost)
                || (path_cost_tbl[neighbor_index] < 0 && -path_cost_tbl[neighbor_index] > cost)
                || path_cost_tbl[neighbor_index] == 0) {
                path_cost_tbl[neighbor_index] = cost;
                path_backtrack_tbl[neighbor_index] = current_index;
            }
        }

        // Mark the current node as processed by negating its cost.
        path_cost_tbl[current_index] = -path_cost_tbl[current_index];
    }

    // Count the number of steps from the current location up to the origin.
    int step = 0;
    int prev_index = path_backtrack_tbl[current_index];
    while (prev_index != -1) {
        prev_index = path_backtrack_tbl[prev_index];
        step++;
    }

    // If the number of steps exceeds allowed maximum, there is no valid path.
    if (step > path_create_info->max_rotations) {
        if (timestamp != 0) {
            dword_5DE600 += tig_timer_elapsed(timestamp);
        }

        return 0;
    }

    // Reconstruct the path backwards from dst to src.
    for (int i = step - 1; i >= 0; i--) {
        path_create_info->rotations[i] = sub_420110(path_backtrack_tbl[target_index], target_index, 64);
        target_index = path_backtrack_tbl[target_index];
    }

    if ((path_create_info->flags & PATH_FLAG_0x0001) != 0) {
        step--;
    }

    if (timestamp != 0) {
        dword_5DE600 += tig_timer_elapsed(timestamp);
    }

    // Return the number of steps in the computed path.
    return step;
}

// 0x4200C0
int path_dist(int src, int dst, int width)
{
    int dx;
    int dy;

    dx = src % width - dst % width;
    if (dx < 0) {
        dx = -dx;
    }

    dy = src / width - dst / width;
    if (dy < 0) {
        dy = -dy;
    }

    if (dx > dy) {
        return 10 * dx;
    } else {
        return 10 * dy;
    }
}

// 0x420110
int sub_420110(int a1, int a2, int a3)
{
    int v3;
    int v4;
    int rot;

    v3 = a2 % a3 - a1 % a3;
    v4 = a2 / a3 - a1 / a3;

    if (v4 >= 0) {
        if (v3 >= 0) {
            if (v4 <= 0) {
                if (v4 > -1)
                    rot = (v4 < 1) + 4;
                else
                    rot = 6;
            } else {
                rot = (v3 >= 1) + 3;
            }
        } else if (v4 > -1) {
            rot = (v4 >= 1) + 1;
        } else {
            rot = 0;
        }
    } else if (v3 > -1) {
        rot = (v3 < 1) + 6;
    } else {
        rot = 0;
    }

    return rot;
}

// 0x4201C0
int sub_4201C0(int64_t from, int64_t to, int8_t* rotations)
{
    S420330 v1;
    int64_t from_x;
    int64_t from_y;
    int64_t to_x;
    int64_t to_y;

    v1.field_0 = 0;
    v1.field_4 = 10;
    v1.field_8 = 0;
    v1.field_10 = 0;
    v1.field_18 = 0;
    v1.rotations = rotations;

    location_xy(from, &from_x, &from_y);
    from_x += 40;
    from_y += 20;

    location_xy(to, &to_x, &to_y);
    to_x += 40;
    to_y += 20;

    v1.field_10 = from_x;
    v1.field_18 = from_y;

    sub_4203B0(from_x, from_y, to_x, to_y, &v1, sub_420330);

    if (v1.field_8 == -1) {
        return 0;
    }

    while (v1.field_0 != 0) {
        sub_420330(to_x, to_y, &v1);
    }

    if (v1.field_8 == -1) {
        return 0;
    }

    return v1.field_8;
}

// 0x420330
void sub_420330(int64_t x, int64_t y, S420330* a5)
{
    if (a5->field_8 == -1 || a5->field_8 >= 200) {
        a5->field_8 = -1;
    } else {
        a5->field_0++;
        if (a5->field_0 == a5->field_4) {
            a5->rotations[a5->field_8] = (x & 0xFF) - (a5->field_10 & 0xFF);
            a5->rotations[a5->field_8 + 1] = (y & 0xFF) - (a5->field_18 & 0xFF);
            a5->field_10 = x;
            a5->field_18 = y;
            a5->field_8 += 2;
            a5->field_0 = 0;
        }
    }
}

// 0x4203B0
void sub_4203B0(int64_t from_x, int64_t from_y, int64_t to_x, int64_t to_y, S420330* a5, void (*fn)(int64_t, int64_t, S420330*))
{
    int64_t dx;
    int64_t dy;
    int64_t ddx;
    int64_t ddy;
    int64_t err;
    int sx;
    int sy;

    dx = to_x - from_x;
    ddx = 2 * llabs(dx);

    if (dx < 0) {
        sx = -1;
    } else if (dx > 0) {
        sx = 1;
    } else {
        sx = 0;
    }

    dy = to_y - from_y;
    ddy = 2 * llabs(dy);

    if (dy < 0) {
        sy = -1;
    } else if (dy > 0) {
        sy = 1;
    } else {
        sy = 0;
    }

    if (ddx <= ddy) {
        err = ddx - ddy / 2;
        fn(from_x, from_y, a5);
        while (from_y != to_y) {
            if (err >= 0) {
                from_x += sx;
                err -= ddy;
            }

            err += ddx;
            from_y += sy;
            fn(from_x, from_y, a5);
        }
    } else {
        err = ddy - ddx / 2;
        fn(from_x, from_y, a5);
        while (from_x != to_x) {
            if (err >= 0) {
                from_y += sy;
                err -= ddx;
            }

            err += ddy;
            from_x += sx;
            fn(from_x, from_y, a5);
        }
    }
}

// 0x420660
int sub_420660(int64_t from, int64_t to, int8_t* rotations)
{
    S420330 v1;
    int64_t from_x;
    int64_t from_y;
    int64_t to_x;
    int64_t to_y;

    v1.field_0 = 0;
    v1.field_4 = 10;
    v1.field_8 = 0;
    v1.field_10 = 0;
    v1.field_18 = 0;
    v1.rotations = NULL;

    location_xy(from, &from_x, &from_y);
    from_x += 40;
    from_y += 20;

    location_xy(to, &to_x, &to_y);
    to_x += 40;
    to_y += 20;

    v1.field_10 = from_x;
    v1.field_18 = from_y;
    sub_4203B0(from_x, from_y, to_x, to_y, &v1, sub_420330);

    if (v1.field_8 == -1) {
        return 0;
    }

    while (v1.field_0 != 0) {
        sub_420330(to_x, to_y, &v1);
    }

    if (v1.field_8 == -1) {
        return 0;
    }

    return v1.field_8;
}

// 0x4207D0
int wmap_path_make(WmapPathInfo* path_info)
{
    int idx;
    int64_t sec = path_info->from;
    int64_t adjacent_sec;
    int rot;
    WmapPathInfo next_path_info;
    int len;

    for (idx = 0; idx < path_info->max_rotations; idx++) {
        if (sec == path_info->to) {
            break;
        }

        rot = sector_rot(sec, path_info->to);
        path_info->rotations[idx] = rot;
        sector_in_dir(sec, rot, &adjacent_sec);

        if (sector_is_blocked(adjacent_sec)
            || (!sector_exists(adjacent_sec) && terrain_is_blocked(adjacent_sec))) {
            next_path_info = *path_info;
            next_path_info.from = sec;
            next_path_info.max_rotations -= idx;
            next_path_info.rotations += idx;
            len = sub_420900(&next_path_info);
            if (len == 0) {
                break;
            }

            idx += len;
            adjacent_sec = next_path_info.to;
        }

        sec = adjacent_sec;
    }

    if (sec != path_info->to) {
        path_info->field_18 = idx;
        return 0;
    }

    return idx;
}

// 0x420900
int sub_420900(WmapPathInfo* path_info)
{
    int64_t sec = path_info->from;
    int64_t adjacent_sec;
    int rot;

    while (sec != path_info->to) {
        rot = sector_rot(sec, path_info->to);
        sector_in_dir(sec, rot, &adjacent_sec);

        if (!sector_is_blocked(adjacent_sec)
            && (sector_exists(adjacent_sec) || !terrain_is_blocked(adjacent_sec))) {
            path_info->to = adjacent_sec;
            return sub_4209C0(path_info);
        }

        sec = adjacent_sec;
    }

    return 0;
}

// 0x4209C0
int sub_4209C0(WmapPathInfo* path_info)
{
    int64_t from_x = SECTOR_X(path_info->from);
    int64_t from_y = SECTOR_Y(path_info->from);
    int64_t to_x = SECTOR_X(path_info->to);
    int64_t to_y = SECTOR_Y(path_info->to);
    int64_t origin_x;
    int64_t origin_y;
    int64_t dx;
    int64_t dy;
    int start_index;
    int target_index;
    int current_index;
    int best_estimated_cost;

    if (from_x > to_x) {
        dx = from_x - to_x;
        origin_x = to_x;
    } else {
        dx = to_x - from_x;
        origin_x = from_x;
    }

    if (from_y > to_y) {
        dy = from_y - to_y;
        origin_y = to_y;
    } else {
        dy = to_y - from_y;
        origin_y = from_y;
    }

    // Check if the sectors are too far apart.
    if (dx >= 8 || dy >= 8) {
        return 0;
    }

    origin_x -= (16 - dx) / 2;
    origin_y -= (16 - dy) / 2;

    start_index = (int)(from_x + (from_y - origin_y) * 16 - origin_x);
    target_index = (int)(to_x + (to_y - origin_y) * 16 - origin_x);

    // Initialize the cost array to zero (unprocessed state).
    memset(wmap_path_cost_tbl, 0, sizeof(wmap_path_cost_tbl));
    wmap_path_cost_tbl[start_index] = 1;
    wmap_path_backtrack_tbl[start_index] = -1;

    while (true) {
        current_index = -1;

        // Grab open node with minimal cost.
        for (int i = 0; i < 256; i++) {
            // Check if node is open, i.e. it has a positive cost (negative cost
            // are closed nodes, zero cost are unprocessed nodes).
            if (wmap_path_cost_tbl[i] > 0) {
                int estimated_cost = wmap_path_cost_tbl[i] + path_dist(i, target_index, 16);
                if (estimated_cost / 10 <= path_info->max_rotations) {
                    // If no candidate node selected yet, or this candidate has
                    // a lower cost, update current candidate.
                    if (current_index == -1 || estimated_cost < best_estimated_cost) {
                        best_estimated_cost = estimated_cost;
                        current_index = i;
                    }
                } else {
                    // Mark node as unreachable.
                    wmap_path_cost_tbl[i] = -32768;
                }
            }
        }

        // If no node has been found, path is not reachable.
        if (current_index == -1) {
            return 0;
        }

        // Check if we have reached the target.
        if (current_index == target_index) {
            break;
        }

        for (int rot = 0; rot < 8; rot++) {
            int64_t sec = SECTOR_MAKE(origin_x + current_index % 16, origin_y + current_index / 16);
            int64_t adjacent_sec;
            if (!sector_in_dir(sec, rot, &adjacent_sec)) {
                continue;
            }

            dx = SECTOR_X(adjacent_sec) - origin_x;
            dy = SECTOR_Y(adjacent_sec) - origin_y;

            if (dx < 0 || dx >= 16 || dy < 0 || dy >= 16) {
                continue;
            }

            int neighbor_index = (int)dx + (int)dy * 16;

            // Skip if the neighbor has already been marked as unreachable.
            if (wmap_path_cost_tbl[neighbor_index] == -32768) {
                continue;
            }

            // Skip blocked sectors or sectors that do not exist.
            if (sector_is_blocked(adjacent_sec)
                || (!sector_exists(adjacent_sec) && terrain_is_blocked(adjacent_sec))) {
                wmap_path_cost_tbl[neighbor_index] = -32768;
                continue;
            }

            // Base movement cost.
            int cost = 10;

            // Add accumulated cost to move the current node.
            cost += wmap_path_cost_tbl[current_index];

            // Add additional cost in case direction change is needed.
            if (wmap_path_backtrack_tbl[current_index] != -1
                && sub_420110(wmap_path_backtrack_tbl[current_index], current_index, 16) != rot) {
                cost++;
            }

            // If this neighbor has not been reached or a lower cost is now
            // available, update its cost and record the current node as its
            // predecessor.
            if ((wmap_path_cost_tbl[neighbor_index] > 0 && wmap_path_cost_tbl[neighbor_index] > cost)
                || (wmap_path_cost_tbl[neighbor_index] < 0 && -wmap_path_cost_tbl[neighbor_index] > cost)
                || wmap_path_cost_tbl[neighbor_index] == 0) {
                wmap_path_cost_tbl[neighbor_index] = cost;
                wmap_path_backtrack_tbl[neighbor_index] = current_index;
            }
        }

        // Mark the current node as processed by negating its cost.
        wmap_path_cost_tbl[current_index] = -wmap_path_cost_tbl[current_index];
    }

    // Count the number of steps from the current location up to the origin.
    int step = 0;
    int prev_index = wmap_path_backtrack_tbl[current_index];
    while (prev_index != -1) {
        prev_index = wmap_path_backtrack_tbl[prev_index];
        step++;
    }

    // If the number of steps exceeds allowed maximum, there is no valid path.
    if (step > path_info->max_rotations) {
        return 0;
    }

    // Reconstruct the path backwards from dst to src.
    for (int i = step - 1; i >= 0; i--) {
        path_info->rotations[i] = sub_420110(wmap_path_backtrack_tbl[target_index], target_index, 16);
        target_index = wmap_path_backtrack_tbl[target_index];
    }

    // Return the number of steps in the computed path.
    return step;
}

// 0x420E30
int sub_420E30(PathCreateInfo* path_create_info, tig_duration_t ms)
{
    // TODO: Incomplete.
}

// 0x421AC0
bool path_set_limit(int value)
{
    if (value <= 0 || value >= 35) {
        return false;
    }

    path_limit = value;

    return true;
}

// 0x421AE0
bool path_set_time_limit(int value)
{
    if (value <= 0 || value >= 1000) {
        return false;
    }

    path_time_limit = value;

    return true;
}
