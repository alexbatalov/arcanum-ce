#include "ui/slide_ui.h"

#include <stdio.h>

#include "game/gamelib.h"
#include "game/gfade.h"
#include "game/gsound.h"
#include "game/hrp.h"
#include "game/mes.h"
#include "game/script.h"

/**
 * The maximum number of enqueued slides.
 */
#define MAX_QUEUE_SIZE 100

static void slide_ui_prepare(int type);
static bool slide_ui_do_slide(tig_window_handle_t window_handle, int slide);
static bool slide_ui_get_assets(int slide, char* bmp_path, char* speech_path);
static void slide_ui_fade_out();
static void slide_ui_fade_in();
static void slide_ui_queue_clear();

/**
 * Current number of slides in the queue.
 *
 * 0x67B970
 */
static int slide_ui_queue_size;

/**
 * "slide.mes"
 *
 * 0x67B974
 */
static mes_file_handle_t slide_ui_slide_mes_file;

/**
 * Array storing the numbers of queued slides.
 *
 * 0x67B978
 */
static int slide_ui_queue[MAX_QUEUE_SIZE];

/**
 * Flag indicating if the slide UI is active.
 *
 * 0x67BB08
 */
static bool slide_ui_active;

/**
 * Called when a module is being loaded.
 *
 * 0x5695B0
 */
bool slide_ui_mod_load()
{
    mes_load("rules\\slide.mes", &slide_ui_slide_mes_file);
    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x5695D0
 */
void slide_ui_mod_unload()
{
    mes_unload(slide_ui_slide_mes_file);
    slide_ui_slide_mes_file = MES_FILE_HANDLE_INVALID;
}

/**
 * Checks if the slide UI is active.
 *
 * 0x5695F0
 */
bool slide_ui_is_active()
{
    return slide_ui_active;
}

/**
 * Starts slide UI.
 *
 * 0x569600
 */
void slide_ui_start(int type)
{
    TigWindowData window_data;
    tig_window_handle_t window_handle;
    tig_sound_handle_t sound_handle;
    int index;

    slide_ui_active = true;

    // Remove all enqeueued slides.
    slide_ui_queue_clear();

    // Run script associated with slide UI type.
    slide_ui_prepare(type);

    // From `slide.mes`: "Slide 0 is the DEFAULT death slide. If the player dies
    // and no slide is queued by script 998, then slide 0 is used."
    if (type == SLIDE_UI_TYPE_DEATH
        && slide_ui_queue_size == 0
        && slide_ui_slide_mes_file != MES_FILE_HANDLE_INVALID) {
        slide_ui_enqueue(0);
    }

    // Perform initial fade-out.
    slide_ui_fade_out();

    // Display slides if queue is not empty.
    if (slide_ui_queue_size > 0) {
        // Set up window properties.
        window_data.flags = TIG_WINDOW_ALWAYS_ON_TOP;
        window_data.rect.x = 0;
        window_data.rect.y = 0;
        window_data.rect.width = 800;
        window_data.rect.height = 600;
        window_data.background_color = 0;
        hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

        // Create the slide window.
        if (tig_window_create(&window_data, &window_handle) == TIG_OK) {
            gsound_stop_all(25);
            tig_mouse_hide();
            sound_handle = gsound_play_music_indefinitely("sound\\music\\DwarvenMusic.mp3", 25);

            // Present each slide in the queue.
            for (index = 0; index < slide_ui_queue_size; index++) {
                if (!slide_ui_do_slide(window_handle, slide_ui_queue[index])) {
                    // Escape was pressed - stop slideshow.
                    break;
                }
            }

            // Clean up resources.
            tig_sound_destroy(sound_handle);
            tig_window_destroy(window_handle);

            tig_mouse_show();
            tig_window_display();
        }
    }

    // Perform fade-in when we're presenting credits. Otherwise the caller is
    // responsible to remove fading.
    if (type == SLIDE_UI_TYPE_CREDITS) {
        slide_ui_fade_in();
    }

    slide_ui_active = false;
}

/**
 * Configures the slide UI by executing a script for the given type.
 *
 * The script is assumed to call `slide_ui_enqueue` via appropriate action, up
 * to `MAX_QUEUE_SIZE` number of times.
 *
 * 0x569720
 */
void slide_ui_prepare(int type)
{
    Script scr;
    ScriptInvocation invocation;

    scr.num = dword_5A5700[type];

    invocation.script = &scr;
    invocation.attachment_point = SAP_USE;
    invocation.triggerer_obj = OBJ_HANDLE_NULL;
    invocation.attachee_obj = OBJ_HANDLE_NULL;
    invocation.extra_obj = OBJ_HANDLE_NULL;
    invocation.line = 0;
    script_execute(&invocation);
}

/**
 * Performs the display of a single slide.
 *
 * Returns `true` if slideshow show continue, `false` if slide was interrupted
 * by pressing ESC.
 *
 * 0x569770
 */
bool slide_ui_do_slide(tig_window_handle_t window_handle, int slide)
{
    TigMessage msg;
    TigRect rect;
    TigBmp bmp;
    char speech_path[TIG_MAX_PATH];
    tig_sound_handle_t speech_handle;
    bool cont = true;
    bool stop;
    tig_timestamp_t start;
    tig_duration_t elapsed;

    // Set slide rect.
    rect.x = 0;
    rect.y = 0;
    rect.width = 800;
    rect.height = 600;

    if (slide_ui_get_assets(slide, bmp.name, speech_path)) {
        // Load image.
        if (tig_bmp_create(&bmp) == TIG_OK) {
            // Put image into window.
            tig_bmp_copy_to_window(&bmp, &rect, window_handle, &rect);
            tig_bmp_destroy(&bmp);

            // Refresh screen.
            sub_51E850(window_handle);
            tig_window_display();

            // Perform fade-in effect.
            slide_ui_fade_in();

            // Play voiceover.
            speech_handle = gsound_play_voice(speech_path, 0);

            // Clear initial message queue.
            while (tig_message_dequeue(&msg) == TIG_OK) {
                if (msg.type == TIG_MESSAGE_REDRAW) {
                    gamelib_redraw();
                }
            }

            // NOTE: There is an interesting bug related to the slideshow.
            // Occasionally, the slides are played in rapid succession. This
            // happens because the underlying `tig_sound_play_streamed_once`
            // does not start the speech immediately. Instead, it marks the
            // speech handle as active and ready to fade in (even with a fade
            // duration of 0). If the host drains the message queue (below in
            // the loop) too quickly, it may not trigger `tig_sound_update`,
            // which is responsible for starting the stream. Consequently, by
            // the time the iteration completes, the stream may not have started
            // at all, causing `tig_sound_is_playing` to report `false` and
            // immediately ending the loop.
            //
            // The fix is adapted from ToEE, where this function has been moved
            // to TIG as `tig_movie_play_slide`. The solution involves adding a
            // minimum delay of 3 seconds per slide. This delay not only serves
            // as a fallback in case the voiceover is missing for any reason,
            // but it also gives the sound system enough time to start the
            // voiceover (which is typically longer than 3 seconds).
            tig_timer_now(&start);

            stop = false;
            do {
                elapsed = tig_timer_elapsed(start);

                // Pump events.
                tig_ping();

                // Process input.
                while (tig_message_dequeue(&msg) == TIG_OK) {
                    if (msg.type == TIG_MESSAGE_KEYBOARD) {
                        if (msg.data.keyboard.pressed == false) {
                            stop = true;
                            if (msg.data.keyboard.scancode == SDL_SCANCODE_ESCAPE) {
                                // Interrupt slideshow.
                                cont = false;
                            }
                        }
                    } else if (msg.type == TIG_MESSAGE_MOUSE) {
                        if (msg.data.mouse.event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP
                            || msg.data.mouse.event == TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP
                            || msg.data.mouse.event == TIG_MESSAGE_MOUSE_MIDDLE_BUTTON_UP) {
                            // Interrupt this slide.
                            stop = true;
                        }
                    } else if (msg.type == TIG_MESSAGE_REDRAW) {
                        gamelib_redraw();
                    }
                }

                if (stop) {
                    break;
                }
            } while ((speech_handle != TIG_SOUND_HANDLE_INVALID && tig_sound_is_playing(speech_handle)) || elapsed < 3000);

            // Clean up voiceover.
            tig_sound_destroy(speech_handle);
        }

        // Clear remaining message queue.
        while (tig_message_dequeue(&msg) == TIG_OK) {
            if (msg.type == TIG_MESSAGE_REDRAW) {
                gamelib_redraw();
            }
        }

        // Perform fade-out effect.
        slide_ui_fade_out();
    }

    return cont;
}

/**
 * Retrieves asset paths for a slide from the message file.
 *
 * 0x569930
 */
bool slide_ui_get_assets(int slide, char* bmp_path, char* speech_path)
{
    MesFileEntry mes_file_entry;
    char temp[MAX_STRING];
    char* tok;

    // Retrieve message file entry.
    mes_file_entry.num = slide;
    if (!mes_search(slide_ui_slide_mes_file, &mes_file_entry)) {
        return false;
    }

    // Each slide contains BMP file name and MP3 voiceover file name. Each piece
    // is mandatory.
    strcpy(temp, mes_file_entry.str);

    // Build BMP path.
    tok = strtok(temp, " \t\n");
    if (tok == NULL) {
        return false;
    }
    sprintf(bmp_path, "slide\\%s", tok);

    // Build MP3 path.
    tok = strtok(NULL, " \t\n");
    if (tok == NULL) {
        return false;
    }
    sprintf(speech_path, "sound\\speech\\slide\\%s", tok);

    return true;
}

/**
 * Performs a fade-out effect for the slide UI.
 *
 * 0x5699F0
 */
void slide_ui_fade_out()
{
    FadeData fade_data;

    fade_data.flags = 0;
    fade_data.duration = 2.0f;
    fade_data.steps = 48;
    fade_data.color = tig_color_make(0, 0, 0);
    gfade_run(&fade_data);
}

/**
 * Performs a fade-in effect for the slide UI.
 *
 * 0x569A60
 */
void slide_ui_fade_in()
{
    FadeData fade_data;

    fade_data.flags = FADE_IN;
    fade_data.duration = 2.0f;
    fade_data.steps = 48;
    gfade_run(&fade_data);
}

/**
 * Clears the slide queue.
 *
 * 0x569A90
 */
void slide_ui_queue_clear()
{
    slide_ui_queue_size = 0;
}

/**
 * Queues a slide for display.
 *
 * 0x569AA0
 */
void slide_ui_enqueue(int slide)
{
    if (slide_ui_queue_size < MAX_QUEUE_SIZE) {
        slide_ui_queue[slide_ui_queue_size++] = slide;
    }
}
