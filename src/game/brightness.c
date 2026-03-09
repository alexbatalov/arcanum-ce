#include "game/brightness.h"

#include "game/gamelib.h"

static void brightness_changed(void);

/**
 * Called when the game is initialized.
 *
 * 0x4BE0B0
 */
bool brightness_init(GameInitInfo* init_info)
{
    (void)init_info;

    // Register `brightness` setting and initialize it.
    settings_register(&settings, BRIGHTNESS_KEY, "0", brightness_changed);
    brightness_changed();

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4BE0E0
 */
void brightness_exit(void)
{
}

/**
 * Called when `brightness` setting is changed.
 *
 * 0x4BE0F0
 */
void brightness_changed(void)
{
    // 0x5FC4C4
    static bool changing;

    int brightness;

    if (!changing) {
        changing = true;

        // Retrieve the current brightness setting (range 0-9).
        brightness = settings_get_value(&settings, BRIGHTNESS_KEY);

        // Attempt to set the gamma (range 1.0 to 1.9).
        if (tig_video_set_gamma((float)brightness * 0.1f + 1.0f) != TIG_OK) {
            tig_debug_printf("brightness_changed: ERROR: Gamma out of Range!  Attempting to Correct bounds!\n");

            // Clamp brightness to a safe range. The `changing` flag prevents
            // infinite loop when settings invoke this callback again.
            brightness = brightness > 9 ? 9 : 1;
            settings_set_value(&settings, BRIGHTNESS_KEY, brightness);

            // Retry setting the gamma with the corrected value.
            if (tig_video_set_gamma(brightness * 0.1f + 1.0f) != TIG_OK) {
                tig_debug_printf("brightness_changed: ERROR: CORRECTION FAILED: Gamma STILL out of Range!\n");
            }
        }

        changing = false;
    }
}
