#include "game/portal.h"

#include "game/a_name.h"
#include "game/gsound.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/script.h"
#include "game/sfx.h"
#include "game/wall.h"

/**
 * Busts a portal.
 *
 * Busting a window usually changes its appearance. Busting a door completely
 * removes door object from the game.
 *
 * 0x4F06E0
 */
void portal_bust(int64_t portal_obj, int64_t triggerer_obj)
{
    tig_art_id_t art_id;
    unsigned int flags;
    int sound_id;

    art_id = obj_field_int32_get(portal_obj, OBJ_F_CURRENT_AID);
    art_id = a_name_portal_aid_busted_set(art_id);
    if (art_id != TIG_ART_ID_INVALID) {
        // The portal is window.

        // Update portal flags.
        flags = obj_field_int32_get(portal_obj, OBJ_F_PORTAL_FLAGS);
        flags |= OPF_BUSTED;
        obj_field_int32_set(portal_obj, OBJ_F_PORTAL_FLAGS, flags);

        // Update appearance.
        object_set_current_aid(portal_obj, art_id);

        object_script_execute(triggerer_obj, portal_obj, OBJ_HANDLE_NULL, SAP_BUST, 0);

        // CE: Different order.
        sound_id = sfx_misc_sound(portal_obj, MISC_SOUND_BUSTING);
        gsound_play_sfx_on_obj(sound_id, 1, portal_obj);
    } else {
        // The portal is door.
        object_destroy(portal_obj);

        sound_id = sfx_misc_sound(portal_obj, MISC_SOUND_DESTROYING);
        gsound_play_sfx_on_obj(sound_id, 1, portal_obj);
    }
}

/**
 * Deletes a portal object.
 *
 * This function only makes sense in the editor. It's used to "erase" portal
 * from the wall, recalculating adjacent wall aids if necessary.
 *
 * 0x4F0790
 */
void portal_delete(int64_t portal_obj, bool a2)
{
    int64_t loc;
    ObjectList objects;
    ObjectNode* node;

    loc = obj_field_int64_get(portal_obj, OBJ_F_LOCATION);

    // Recalc walls.
    object_list_location(loc, OBJ_TM_WALL, &objects);
    node = objects.head;
    while (node != NULL) {
        wall_delete(node->obj, a2);
        node = node->next;
    }
    object_list_destroy(&objects);

    // Delete portal.
    object_list_location(loc, OBJ_TM_PORTAL, &objects);
    node = objects.head;
    while (node != NULL) {
        sub_43CF70(node->obj);
        object_delete(node->obj);
        node = node->next;
    }
    object_list_destroy(&objects);
}

/**
 * Checks if a portal is in an open state based on its animation frame.
 *
 * 0x4F08C0
 */
bool portal_is_open(int64_t portal_obj)
{
    return tig_art_id_frame_get(obj_field_int32_get(portal_obj, OBJ_F_CURRENT_AID)) != 0;
}

/**
 * Toggles the state of a portal between open and closed.
 *
 * Note that this function uses spinwait technique to play open/close animation.
 * Upon return the portal is guaranteed to be either fully open or closed.
 *
 * 0x4F08F0
 */
void portal_toggle(int64_t portal_obj)
{
    if (tig_art_id_frame_get(obj_field_int32_get(portal_obj, OBJ_F_CURRENT_AID)) == 0) {
        // Portal is closed, wait until it is completely open.
        while (portal_open(portal_obj)) {
        }
    } else {
        // Portal is open, wait until it is completely closed.
        while (portal_close(portal_obj)) {
        }
    }
}

/**
 * Attempts to open a portal, advancing its animation frame and updating flags.
 *
 * Returns `true` if the portal's state was advanced, `false` if it's already
 * open or invalid.
 *
 * 0x4F0950
 */
bool portal_open(int64_t portal_obj)
{
    tig_art_id_t art_id;
    TigArtAnimData art_anim_data;
    int frame;
    int rotation;

    // Make sure the object is a portal.
    if (obj_field_int32_get(portal_obj, OBJ_F_TYPE) != OBJ_TYPE_PORTAL) {
        return false;
    }

    // Retrieve animation data.
    art_id = obj_field_int32_get(portal_obj, OBJ_F_CURRENT_AID);
    if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
        return false;
    }

    // Check if there is open/close animation.
    if (art_anim_data.num_frames == 1) {
        return false;
    }

    frame = tig_art_id_frame_get(art_id);
    if (tig_art_portal_id_type_get(art_id) == TIG_ART_PORTAL_TYPE_WINDOW) {
        // The windows does not have animation, just two frames: closed (0) and
        // open (1).
        if (frame == 1) {
            // Already open.
            return false;
        }

        art_id = tig_art_id_frame_set(art_id, 1);
        object_flags_set(portal_obj, OF_SHOOT_THROUGH | OF_NO_BLOCK);
    } else {
        // The doors have 3 animation frames, arranged at specific numbers
        // depending on the rotation.
        rotation = tig_art_id_rotation_get(art_id);

        // Normalize diagonal directions to cardinal directions.
        if ((rotation & 1) != 0) {
            rotation--;
        }

        if (rotation == 6 || rotation == 0) {
            // West and north directions have animations in frames 4-6.
            if (frame >= 4 && frame <= 6) {
                if (frame == 6) {
                    return false;
                }

                // Advance to the next frame.
                art_id = tig_art_id_frame_set(art_id, frame + 1);
            } else {
                // Start opening animation.
                art_id = tig_art_id_frame_set(art_id, 4);
                object_flags_set(portal_obj, OF_SHOOT_THROUGH | OF_NO_BLOCK | OF_SEE_THROUGH);
            }
        } else {
            // East and south directions have animations in frames 1-3.
            if (frame >= 1 && frame <= 3) {
                if (frame == 3) {
                    return false;
                }

                // Advance to the next frame.
                art_id = tig_art_id_frame_set(art_id, frame + 1);
            } else {
                // Start opening animation.
                art_id = tig_art_id_frame_set(art_id, 1);
                object_flags_set(portal_obj, OF_SHOOT_THROUGH | OF_NO_BLOCK | OF_SEE_THROUGH);
            }
        }
    }

    object_set_current_aid(portal_obj, art_id);

    return true;
}

/**
 * Attempts to close a portal, reversing its animation and updating flags.
 *
 * Returns `true` if the portal's state was advanced, `false` of already closed
 * or invalid.
 *
 * 0x4F0A90
 */
bool portal_close(int64_t portal_obj)
{
    tig_art_id_t art_id;
    TigArtAnimData art_anim_data;
    int frame;
    int rotation;

    // Make sure the object is a portal.
    if (obj_field_int32_get(portal_obj, OBJ_F_TYPE) != OBJ_TYPE_PORTAL) {
        return false;
    }

    // Retrieve animation data.
    art_id = obj_field_int32_get(portal_obj, OBJ_F_CURRENT_AID);
    if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
        return false;
    }

    // Check if there is open/close animation.
    if (art_anim_data.num_frames == 1) {
        return false;
    }

    // Check if the portal is already closed. Frame 0 is closed frame for both
    // windows and doors (all directions).
    frame = tig_art_id_frame_get(art_id);
    if (frame == 0) {
        return false;
    }

    if (tig_art_portal_id_type_get(art_id) == TIG_ART_PORTAL_TYPE_WINDOW) {
        art_id = tig_art_id_frame_set(art_id, 0);
        object_flags_unset(portal_obj, OF_SHOOT_THROUGH | OF_NO_BLOCK);
    } else {
        rotation = tig_art_id_rotation_get(art_id);

        // Normalize diagonal directions to cardinal directions.
        if ((rotation & 1) != 0) {
            rotation--;
        }

        if (rotation == 6 || rotation == 0) {
            // West and north directions have animations in frames 4-6.
            if (frame >= 4 && frame <= 6) {
                if (frame == 4) {
                    // Fully closed, unset flags and set to frame 0.
                    object_flags_unset(portal_obj, OF_SHOOT_THROUGH | OF_NO_BLOCK | OF_SEE_THROUGH);
                    art_id = tig_art_id_frame_set(art_id, 0);
                } else {
                    // Reverse to previous frame.
                    art_id = tig_art_id_frame_set(art_id, frame - 1);
                }
            } else {
                // Start closing animation.
                art_id = tig_art_id_frame_set(art_id, 6);
            }
        } else {
            // East and south directions have animations in frames 1-3.
            if (frame >= 1 && frame <= 3) {
                if (frame == 1) {
                    // Fully closed, unset flags and set to frame 0.
                    object_flags_unset(portal_obj, OF_SHOOT_THROUGH | OF_NO_BLOCK | OF_SEE_THROUGH);
                    art_id = tig_art_id_frame_set(art_id, 0);
                } else {
                    // Reverse to previous frame.
                    art_id = tig_art_id_frame_set(art_id, frame - 1);
                }
            } else {
                // Start closing animation.
                art_id = tig_art_id_frame_set(art_id, 3);
            }
        }
    }

    object_set_current_aid(portal_obj, art_id);

    return true;
}
