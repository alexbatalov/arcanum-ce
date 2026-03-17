#include "ui/tech_ui.h"

#include "game/player.h"
#include "game/stat.h"
#include "game/tech.h"
#include "ui/charedit_ui.h"
#include "ui/intgame.h"

/**
 * Increases the degree of a specified technology for a critter.
 *
 * 0x589A20
 */
void tech_ui_inc_degree(int64_t obj, int tech)
{
    int degree;
    int cost;
    int points;
    bool is_pc;

    // Retrieve current degree, cost for next degree, and available points.
    degree = tech_degree_get(obj, tech);
    cost = tech_degree_cost_get(degree + 1);
    points = stat_level_get(obj, STAT_UNSPENT_POINTS);
    is_pc = player_is_local_pc_obj(obj);

    // Check if there are enough character points to increment the degree.
    if (cost > points) {
        // Notify player of insufficient points.
        if (is_pc) {
            charedit_error_not_enough_character_points();
        }
    } else {
        // Attempt to increase the technology degree.
        if (tech_degree_inc(obj, tech) > degree) {
            // Deduct cost from unspent points.
            stat_base_set(obj, STAT_UNSPENT_POINTS, points - cost);

            // Update UI.
            if (is_pc) {
                charedit_refresh();
                intgame_message_window_clear();
            }
        } else {
            // Notify player of insufficient intelligence.
            //
            // NOTE: In fact there could be other reasons why next degree was
            // not obtained.
            if (is_pc) {
                charedit_error_not_enough_intelligence();
            }
        }
    }
}

/**
 * Decreases the degree of a specified technology for a critter.
 *
 * 0x589AF0
 */
void tech_ui_dec_degree(int64_t obj, int tech)
{
    int degree;
    int cost;
    int points;
    bool is_pc;

    // Retrieve current degree, cost of the current degree, and available
    // points.
    degree = tech_degree_get(obj, tech);
    cost = tech_degree_cost_get(degree);
    points = stat_level_get(obj, STAT_UNSPENT_POINTS);
    is_pc = player_is_local_pc_obj(obj);

    // Attempt to decrease the technology degree.
    if (tech_degree_dec(obj, tech) < degree) {
        // Refund cost of the degree to unspent points.
        stat_base_set(obj, STAT_UNSPENT_POINTS, points + cost);

        // Update UI.
        if (is_pc) {
            charedit_refresh();
            intgame_message_window_clear();
        }
    }
}

/**
 * Called when the multiplayer game receives `mpPacketTypePlayerRequestTechChange`
 * packet.
 *
 * 0x589B90
 */
void tech_ui_adjust_degree(int64_t obj, int tech, int action)
{
    switch (action) {
    case 1:
        tech_ui_inc_degree(obj, tech);
        break;
    case -1:
        tech_ui_dec_degree(obj, tech);
        break;
    }
}
