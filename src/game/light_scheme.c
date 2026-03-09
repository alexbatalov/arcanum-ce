#include "game/light_scheme.h"

#include <stdio.h>

#include "game/light.h"
#include "game/mes.h"

// A lighting scheme contains exactly 24 values - for each hour of the day.
#define LIGHT_SCHEME_HOURS 24

/**
 * Represents RGB color as individual components.
 *
 * NOTE: For unknown reason these values are stored as-as without scaling to
 * current BPP.
 *
 * NOTE: There are no traces of such struct in the code. However it's handy
 * as it reduces number of assignments in `light_scheme_parse` significanlty
 * and improves code readability.
 */
typedef struct LightSchemeColorComponents {
    int red;
    int green;
    int blue;
} LightSchemeColorComponents;

/**
 * Represents light scheme entry, which is a fancy name for a pair of ambient
 * colors for indoor and outdoor locations.
 */
typedef struct LightSchemeData {
    LightSchemeColorComponents indoor;
    LightSchemeColorComponents outdoor;
} LightSchemeData;

static void light_scheme_parse(int msg_file);

/**
 * Stores the currently active light scheme.
 *
 * 0x5B5050
 */
static int light_scheme_current_light_scheme_id = -1;

/**
 * Stores the default light scheme for the map.
 *
 * 0x5B5054
 */
static int light_scheme_map_default = LIGHT_SCHEME_DEFAULT_LIGHTING;

/**
 * Array of light scheme entries for each hour of the day (0-23).
 *
 * 0x5F5CA0
 */
static LightSchemeData* light_scheme_colors;

/**
 * Current hour for the light scheme (0-23).
 *
 * 0x5F5CA4
 */
static int light_scheme_hour;

/**
 * "Lighting Schemes.mes"
 *
 * 0x5F5CA8
 */
static mes_file_handle_t light_schemes_msg_file;

/**
 * Flag indicating if the light scheme is currently being updated.
 *
 * 0x5F5CAC
 */
static bool light_scheme_changing;

/**
 * Called when the game is initialized.
 *
 * 0x4A7C60
 */
bool light_scheme_init(GameInitInfo* init_info)
{
    (void)init_info;

    // Allocate memory for 24 hours of light scheme data.
    light_scheme_colors = (LightSchemeData*)CALLOC(LIGHT_SCHEME_HOURS, sizeof(*light_scheme_colors));

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4A7C80
 */
void light_scheme_reset(void)
{
    // Reset to default light scheme at noon.
    light_scheme_set(LIGHT_SCHEME_MAP_DEFAULT, 12);
}

/**
 * Called when a module is being loaded.
 *
 * 0x4A7C90
 */
bool light_scheme_mod_load(void)
{
    // Load light schemes configuration (required).
    if (!mes_load("Rules\\Lighting Schemes.mes", &light_schemes_msg_file)) {
        return false;
    }

    // Set the default light scheme at noon.
    light_scheme_set(LIGHT_SCHEME_MAP_DEFAULT, 12);

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4A7CC0
 */
void light_scheme_mod_unload(void)
{
    if (light_schemes_msg_file != MES_FILE_HANDLE_INVALID) {
        mes_unload(light_schemes_msg_file);
        light_schemes_msg_file = MES_FILE_HANDLE_INVALID;
    }

    light_scheme_current_light_scheme_id = -1;
}

/**
 * Called when the game shuts down.
 *
 * 0x4A7CF0
 */
void light_scheme_exit(void)
{
    FREE(light_scheme_colors);
}

/**
 * Called when the game is being saved.
 *
 * 0x4A7D00
 */
bool light_scheme_save(TigFile* stream)
{
    if (tig_file_fwrite(&light_scheme_current_light_scheme_id, sizeof(light_scheme_current_light_scheme_id), 1, stream) != 1) return false;
    if (tig_file_fwrite(&light_scheme_hour, sizeof(light_scheme_hour), 1, stream) != 1) return false;

    return true;
}

/**
 * Called when the game is being loaded.
 *
 * 0x4A7D40
 */
bool light_scheme_load(GameLoadInfo* load_info)
{
    int light_scheme;
    int hour;

    if (tig_file_fread(&light_scheme, sizeof(light_scheme), 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&hour, sizeof(hour), 1, load_info->stream) != 1) return false;

    light_scheme_set(light_scheme, hour);

    return true;
}

/**
 * Sets the default light scheme.
 *
 * Returns `true` if lighting scheme is successfully set, or `false` when
 * the specified scheme is invalid.
 *
 * 0x4A7DA0
 */
bool light_scheme_set_map_default(int light_scheme)
{
    if (light_scheme > 0) {
        light_scheme_map_default = light_scheme;
        return true;
    }

    return false;
}

/**
 * Retrieves the default light scheme.
 *
 * 0x4A7DC0
 */
int light_scheme_get_map_default(void)
{
    return light_scheme_map_default;
}

/**
 * Sets the active light scheme and current hour.
 *
 * 0x4A7DD0
 */
bool light_scheme_set(int light_scheme, int hour)
{
    MesFileEntry mes_file_entry;
    char path[TIG_MAX_PATH];
    mes_file_handle_t mes_file;

    // Check if the message file is loaded.
    if (light_schemes_msg_file == MES_FILE_HANDLE_INVALID) {
        return false;
    }

    // Resolve special value of `LIGHT_SCHEME_MAP_DEFAULT` to the actual default
    // scheme.
    if (light_scheme == LIGHT_SCHEME_MAP_DEFAULT) {
        light_scheme = light_scheme_map_default;
    }

    // Check if the requested light scheme is already loaded.
    if (light_scheme != light_scheme_current_light_scheme_id) {
        // Search for the light scheme file name in the message file.
        mes_file_entry.num = light_scheme;
        if (mes_search(light_schemes_msg_file, &mes_file_entry)) {
            sprintf(path, "Rules\\%s.mes", mes_file_entry.str);

            // Load and parse the light scheme file.
            if (mes_load(path, &mes_file)) {
                light_scheme_parse(mes_file);
                mes_unload(mes_file);

                // Save the loaded light scheme.
                light_scheme_current_light_scheme_id = light_scheme;
            }
        }
    }

    // Verify if the scheme was successfully applied.
    if (light_scheme != light_scheme_current_light_scheme_id) {
        return false;
    }

    return light_scheme_set_hour(hour);
}

/**
 * Retrieves the current light scheme.
 *
 * 0x4A7EA0
 */
int light_scheme_get(void)
{
    return light_scheme_current_light_scheme_id;
}

/**
 * Sets the hour for the current light scheme.
 *
 * 0x4A7EB0
 */
bool light_scheme_set_hour(int hour)
{
    LightSchemeData* light_scheme_data;
    int indoor_color;
    int outdoor_color;

    // Check if the message file is loaded.
    if (light_schemes_msg_file == MES_FILE_HANDLE_INVALID) {
        return false;
    }

    // Validate the hour range.
    if (hour < 0 || hour >= LIGHT_SCHEME_HOURS) {
        return false;
    }

    light_scheme_hour = hour;

    // Indicate that the light scheme is being updated.
    light_scheme_changing = true;

    // Retrieve color data for the specified hour.
    light_scheme_data = &(light_scheme_colors[hour]);
    indoor_color = tig_color_make(light_scheme_data->indoor.red, light_scheme_data->indoor.green, light_scheme_data->indoor.blue);
    outdoor_color = tig_color_make(light_scheme_data->outdoor.red, light_scheme_data->outdoor.green, light_scheme_data->outdoor.blue);

    // Update ambient colors.
    light_set_colors(indoor_color, outdoor_color);

    // Indicate that the update is complete.
    light_scheme_changing = false;

    return true;
}

/**
 * Retrieves the current hour of the light scheme.
 *
 * 0x4A7FA0
 */
int light_scheme_get_hour(void)
{
    return light_scheme_hour;
}

/**
 * Checks if the light scheme is currently being updated.
 *
 * 0x4A7FB0
 */
int light_scheme_is_changing(void)
{
    return light_scheme_changing;
}

/**
 * Parses a message file to populate the light scheme colors for each hour.
 *
 * 0x4A8060
 */
void light_scheme_parse(int msg_file)
{
    bool interpolate = false;
    int prev_index = 0;
    LightSchemeColorComponents outdoor;
    LightSchemeColorComponents indoor;
    LightSchemeColorComponents prev_outdoor;
    LightSchemeColorComponents prev_indoor;
    int index;
    MesFileEntry mes_file_entry;
    char* str;

    // NOTE: Initialize colors to zero to keep compiler happy. Original code
    // does not perform this initialization.
    memset(&outdoor, 0, sizeof(outdoor));
    memset(&indoor, 0, sizeof(indoor));
    memset(&prev_outdoor, 0, sizeof(prev_outdoor));
    memset(&prev_indoor, 0, sizeof(prev_indoor));

    tig_str_parse_set_separator(',');

    // Iterate through each of 24 hours. Every message is either have RGB colors
    // specified or an empty string indicating that the value for this hour
    // should be interpolated from the closest non-empty settings.
    for (index = 0; index < LIGHT_SCHEME_HOURS; index++) {
        mes_file_entry.num = index;

        if (mes_search(msg_file, &mes_file_entry) && mes_file_entry.str[0] != '\0') {
            // Parse color values.
            str = mes_file_entry.str;
            tig_str_parse_value(&str, &(outdoor.red));
            tig_str_parse_value(&str, &(outdoor.green));
            tig_str_parse_value(&str, &(outdoor.blue));
            tig_str_parse_value(&str, &(indoor.red));
            tig_str_parse_value(&str, &(indoor.green));
            tig_str_parse_value(&str, &(indoor.blue));

            // Store colors corresponding to current hour.
            light_scheme_colors[index].outdoor = outdoor;
            light_scheme_colors[index].indoor = indoor;

            // Check if interpolation from previous known values is needed.
            if (interpolate) {
                // Calculate step sizes for linear interpolation. Each color is
                // channel interpolated independently.
                int outdoor_red_step = (outdoor.red - prev_outdoor.red) / (index - prev_index);
                int outdoor_green_step = (outdoor.green - prev_outdoor.green) / (index - prev_index);
                int outdoor_blue_step = (outdoor.blue - prev_outdoor.blue) / (index - prev_index);
                int indoor_red_step = (indoor.red - prev_indoor.red) / (index - prev_index);
                int indoor_green_step = (indoor.green - prev_indoor.green) / (index - prev_index);
                int indoor_blue_step = (indoor.blue - prev_indoor.blue) / (index - prev_index);
                int prev;

                // Start with the previous colors.
                outdoor = prev_outdoor;
                indoor = prev_indoor;

                // Fill in the missing hours with interpolated colors.
                for (prev = prev_index; prev < index; prev++) {
                    light_scheme_colors[prev].outdoor = outdoor;
                    light_scheme_colors[prev].indoor = indoor;

                    outdoor.red += outdoor_red_step;
                    outdoor.green += outdoor_green_step;
                    outdoor.blue += outdoor_blue_step;
                    indoor.red += indoor_red_step;
                    indoor.green += indoor_green_step;
                    indoor.blue += indoor_blue_step;
                }

                interpolate = false;
            }

            // Store the index of the last non-empty string.
            prev_index = index;
        } else {
            // The string is empty - store last read settings as previous and
            // set a flag indicating interpolation is required.
            prev_outdoor = outdoor;
            prev_indoor = indoor;
            interpolate = true;
        }
    }
}
