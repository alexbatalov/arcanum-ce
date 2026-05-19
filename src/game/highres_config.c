#include "game/highres_config.h"

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL_stdinc.h>

// Keys in arcanum.cfg that mirror HighRes/config.ini. arcanum.cfg uses
// lowercase keys with spaces; we match that convention for consistency.
#define CFG_KEY_WIDTH       "resolution width"
#define CFG_KEY_HEIGHT      "resolution height"
#define CFG_KEY_WINDOWED    "windowed"
#define CFG_KEY_SHOW_FPS    "show fps"
#define CFG_KEY_SCROLL_FPS  "scroll fps"
#define CFG_KEY_SCROLL_DIST "scroll dist"
#define CFG_KEY_LOGOS       "logos"
#define CFG_KEY_INTRO       "intro"
#define CFG_KEY_IGNORE_NOTCH "macos ignore notch"
#define CFG_KEY_ASPECT_SNAP  "aspect snap"

static void highres_config_reset(void);
static void highres_config_parse_line(char* line);
static void highres_config_trim(char** start_ptr, char** end_ptr);
static bool highres_config_parse_int(const char* str, int* value_ptr);
static bool highres_config_load_from_arcanum_cfg(void);
static bool highres_config_load_from_highres_ini(void);
static void highres_config_migrate_to_arcanum_cfg(void);
static bool arcanum_cfg_read_int(const char* key, int* out);
static bool arcanum_cfg_write_ints(const char* const* keys, const int* values, int count);

static HighResConfig highres_config;

void highres_config_load(void)
{
    highres_config_reset();

    // Preferred source: arcanum.cfg. If any of the resolution keys are
    // present we treat arcanum.cfg as authoritative and read everything we
    // care about from there.
    if (highres_config_load_from_arcanum_cfg()) {
        highres_config.loaded = true;
        return;
    }

    // First-launch fallback: pull values from the legacy HighRes/config.ini
    // produced by the High Resolution Patch, then migrate them into
    // arcanum.cfg so this file becomes the system of record going forward.
    if (highres_config_load_from_highres_ini()) {
        highres_config.loaded = true;
        highres_config_migrate_to_arcanum_cfg();
    }
}

const HighResConfig* highres_config_get(void)
{
    return &highres_config;
}

bool highres_config_save_resolution(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }

    const char* keys[] = { CFG_KEY_WIDTH, CFG_KEY_HEIGHT };
    const int values[] = { width, height };
    if (!arcanum_cfg_write_ints(keys, values, 2)) {
        return false;
    }

    highres_config.width = width;
    highres_config.height = height;
    return true;
}

static bool highres_config_load_from_arcanum_cfg(void)
{
    int width = 0;
    bool found_width = arcanum_cfg_read_int(CFG_KEY_WIDTH, &width);
    if (!found_width) {
        return false;
    }

    int height = 0;
    if (arcanum_cfg_read_int(CFG_KEY_HEIGHT, &height) && height >= 600) {
        highres_config.height = height;
    }
    if (width >= 800) {
        highres_config.width = width;
    }

    int value;
    if (arcanum_cfg_read_int(CFG_KEY_WINDOWED, &value)) {
        highres_config.windowed = value != 0;
    }
    if (arcanum_cfg_read_int(CFG_KEY_SHOW_FPS, &value)) {
        highres_config.show_fps = value != 0;
    }
    if (arcanum_cfg_read_int(CFG_KEY_SCROLL_FPS, &value) && value > 0) {
        highres_config.scroll_fps = value;
    }
    if (arcanum_cfg_read_int(CFG_KEY_SCROLL_DIST, &value) && value >= 0) {
        highres_config.scroll_dist = value;
    }
    if (arcanum_cfg_read_int(CFG_KEY_LOGOS, &value)) {
        highres_config.logos = value != 0;
    }
    if (arcanum_cfg_read_int(CFG_KEY_INTRO, &value)) {
        highres_config.intro = value != 0;
    }
    if (arcanum_cfg_read_int(CFG_KEY_IGNORE_NOTCH, &value)) {
        highres_config.ignore_notch = value != 0;
    }
    if (arcanum_cfg_read_int(CFG_KEY_ASPECT_SNAP, &value)) {
        highres_config.aspect_snap = value != 0;
    }
    return true;
}

static bool highres_config_load_from_highres_ini(void)
{
    FILE* stream = fopen("HighRes/config.ini", "rt");
    if (stream == NULL) {
        return false;
    }

    char line[512];
    while (fgets(line, sizeof(line), stream) != NULL) {
        highres_config_parse_line(line);
    }

    fclose(stream);
    return true;
}

static void highres_config_migrate_to_arcanum_cfg(void)
{
    const char* keys[] = {
        CFG_KEY_WIDTH,
        CFG_KEY_HEIGHT,
        CFG_KEY_WINDOWED,
        CFG_KEY_SHOW_FPS,
        CFG_KEY_SCROLL_FPS,
        CFG_KEY_SCROLL_DIST,
        CFG_KEY_LOGOS,
        CFG_KEY_INTRO,
        CFG_KEY_IGNORE_NOTCH,
        CFG_KEY_ASPECT_SNAP,
    };
    const int values[] = {
        highres_config.width,
        highres_config.height,
        highres_config.windowed ? 1 : 0,
        highres_config.show_fps ? 1 : 0,
        highres_config.scroll_fps,
        highres_config.scroll_dist,
        highres_config.logos ? 1 : 0,
        highres_config.intro ? 1 : 0,
        highres_config.ignore_notch ? 1 : 0,
        highres_config.aspect_snap ? 1 : 0,
    };
    arcanum_cfg_write_ints(keys, values, (int)(sizeof(keys) / sizeof(keys[0])));
}

static bool arcanum_cfg_read_int(const char* key, int* out)
{
    FILE* stream = fopen("arcanum.cfg", "rt");
    if (stream == NULL) {
        return false;
    }

    size_t key_len = strlen(key);
    char line[512];
    bool found = false;
    while (fgets(line, sizeof(line), stream) != NULL) {
        char* scan = line;
        while (*scan == ' ' || *scan == '\t') {
            scan++;
        }
        if (*scan == '\0' || *scan == '\r' || *scan == '\n' || (scan[0] == '/' && scan[1] == '/')) {
            continue;
        }
        char* eq = strchr(scan, '=');
        if (eq == NULL) {
            continue;
        }
        // Trim trailing whitespace from the key portion.
        char* key_end = eq;
        while (key_end > scan && (key_end[-1] == ' ' || key_end[-1] == '\t')) {
            key_end--;
        }
        if ((size_t)(key_end - scan) != key_len) {
            continue;
        }
        if (SDL_strncasecmp(scan, key, key_len) != 0) {
            continue;
        }
        if (highres_config_parse_int(eq + 1, out)) {
            found = true;
        }
        break;
    }

    fclose(stream);
    return found;
}

static bool arcanum_cfg_write_ints(const char* const* keys, const int* values, int count)
{
    if (count <= 0) {
        return true;
    }

    // Read the existing file (if any) into memory so we can preserve every
    // line we don't intend to touch.
    FILE* in = fopen("arcanum.cfg", "rt");

    FILE* out = fopen("arcanum.cfg.tmp", "wt");
    if (out == NULL) {
        if (in != NULL) {
            fclose(in);
        }
        return false;
    }

    bool* written = (bool*)calloc((size_t)count, sizeof(bool));
    if (written == NULL) {
        if (in != NULL) {
            fclose(in);
        }
        fclose(out);
        remove("arcanum.cfg.tmp");
        return false;
    }

    if (in != NULL) {
        char line[512];
        while (fgets(line, sizeof(line), in) != NULL) {
            char* scan = line;
            while (*scan == ' ' || *scan == '\t') {
                scan++;
            }
            if (*scan == '\0' || *scan == '\r' || *scan == '\n' || (scan[0] == '/' && scan[1] == '/')) {
                fputs(line, out);
                continue;
            }
            char* eq = strchr(scan, '=');
            if (eq == NULL) {
                fputs(line, out);
                continue;
            }
            char* key_end = eq;
            while (key_end > scan && (key_end[-1] == ' ' || key_end[-1] == '\t')) {
                key_end--;
            }
            size_t line_key_len = (size_t)(key_end - scan);

            int matched = -1;
            for (int i = 0; i < count; i++) {
                size_t k = strlen(keys[i]);
                if (k == line_key_len && SDL_strncasecmp(scan, keys[i], k) == 0) {
                    matched = i;
                    break;
                }
            }
            if (matched >= 0 && !written[matched]) {
                fprintf(out, "%s=%d\n", keys[matched], values[matched]);
                written[matched] = true;
            } else {
                fputs(line, out);
            }
        }
        fclose(in);
    }

    // Append any keys that weren't already present.
    for (int i = 0; i < count; i++) {
        if (!written[i]) {
            fprintf(out, "%s=%d\n", keys[i], values[i]);
        }
    }

    free(written);
    fclose(out);

    if (remove("arcanum.cfg") != 0 && errno != ENOENT) {
        remove("arcanum.cfg.tmp");
        return false;
    }
    if (rename("arcanum.cfg.tmp", "arcanum.cfg") != 0) {
        return false;
    }
    return true;
}

void highres_config_reset(void)
{
    highres_config.loaded = false;
    highres_config.width = 800;
    highres_config.height = 600;
    highres_config.windowed = false;
    highres_config.show_fps = false;
    highres_config.scroll_fps = 35;
    highres_config.scroll_dist = 10;
    highres_config.logos = true;
    highres_config.intro = true;
    highres_config.ignore_notch = false;
    highres_config.aspect_snap = true;
}

void highres_config_parse_line(char* line)
{
    char* comment;
    char* sep;
    char* key_start;
    char* key_end;
    char* value_start;
    char* value_end;
    int value;

    comment = strstr(line, "//");
    if (comment != NULL) {
        *comment = '\0';
    }

    key_start = line;
    key_end = key_start + strlen(key_start);
    highres_config_trim(&key_start, &key_end);
    *key_end = '\0';

    if (*key_start == '\0') {
        return;
    }

    sep = strchr(key_start, '=');
    if (sep == NULL) {
        return;
    }

    *sep = '\0';

    key_end = sep;
    highres_config_trim(&key_start, &key_end);
    *key_end = '\0';

    value_start = sep + 1;
    value_end = value_start + strlen(value_start);
    highres_config_trim(&value_start, &value_end);
    *value_end = '\0';

    if (!highres_config_parse_int(value_start, &value)) {
        return;
    }

    if (SDL_strcasecmp(key_start, "Width") == 0) {
        if (value >= 800) {
            highres_config.width = value;
        }
    } else if (SDL_strcasecmp(key_start, "Height") == 0) {
        if (value >= 600) {
            highres_config.height = value;
        }
    } else if (SDL_strcasecmp(key_start, "Windowed") == 0) {
        highres_config.windowed = value != 0;
    } else if (SDL_strcasecmp(key_start, "ShowFPS") == 0) {
        highres_config.show_fps = value != 0;
    } else if (SDL_strcasecmp(key_start, "ScrollFPS") == 0) {
        if (value > 0) {
            highres_config.scroll_fps = value;
        }
    } else if (SDL_strcasecmp(key_start, "ScrollDist") == 0) {
        if (value >= 0) {
            highres_config.scroll_dist = value;
        }
    } else if (SDL_strcasecmp(key_start, "Logos") == 0) {
        highres_config.logos = value != 0;
    } else if (SDL_strcasecmp(key_start, "Intro") == 0) {
        highres_config.intro = value != 0;
    } else if (SDL_strcasecmp(key_start, "IgnoreNotch") == 0) {
        highres_config.ignore_notch = value != 0;
    } else if (SDL_strcasecmp(key_start, "AspectSnap") == 0) {
        highres_config.aspect_snap = value != 0;
    }
}

void highres_config_trim(char** start_ptr, char** end_ptr)
{
    while (*start_ptr < *end_ptr && isspace((unsigned char)**start_ptr)) {
        (*start_ptr)++;
    }

    while (*end_ptr > *start_ptr && isspace((unsigned char)*((*end_ptr) - 1))) {
        (*end_ptr)--;
    }
}

bool highres_config_parse_int(const char* str, int* value_ptr)
{
    char* end;
    long value;

    if (*str == '\0') {
        return false;
    }

    value = strtol(str, &end, 10);
    if (end == str) {
        return false;
    }

    while (*end != '\0') {
        if (!isspace((unsigned char)*end)) {
            return false;
        }

        end++;
    }

    *value_ptr = (int)value;
    return true;
}
