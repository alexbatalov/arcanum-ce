#include "game/settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/obj_private.h"

static SettingsEntry* settings_find_ex(Settings* settings, const char* section, const char* key);
static void settings_register_ex(Settings* settings, const char* section, const char* key, const char* default_value, SettingsValueChangedFunc value_changed_func);
static void settings_set_str_value_ex(Settings* settings, const char* section, const char* key, const char* value, bool mark_changed, bool notify);
static bool settings_section_equals(const char* lhs, const char* rhs);
static bool settings_section_was_written(Settings* settings, SettingsEntry* entry);
static void settings_trim(char* str);

/**
 * Initializes a settings structure with a file path.
 *
 * 0x438AE0
 */
void settings_init(Settings* settings, const char* path)
{
    settings->path = path;
    settings->entries = NULL;
    settings->flags = 0;
}

/**
 * Frees all resources associated with a settings structure.
 *
 * 0x438B00
 */
void settings_exit(Settings* settings)
{
    SettingsEntry* curr;
    SettingsEntry* next;

    curr = settings->entries;
    while (curr != NULL) {
        next = curr->next;
        FREE(curr->section);
        FREE(curr->key);
        FREE(curr->value);
        FREE(curr);
        curr = next;
    }

    settings->path = NULL;
    settings->entries = NULL;
}

/**
 * Loads settings from the associated file.
 *
 * File path is specified at creation time with `settings_init` and should
 * not be changed.
 *
 * 0x438B50
 */
void settings_load(Settings* settings)
{
    FILE* stream;
    char buffer[256];
    char section[64];
    char* sep;

    stream = fopen(settings->path, "rt");
    if (stream == NULL) {
        return;
    }

    section[0] = '\0';

    while (fgets(buffer, sizeof(buffer), stream) != NULL) {
        settings_trim(buffer);
        if (buffer[0] == '\0') {
            continue;
        }

        if (buffer[0] == '[') {
            char* end = strchr(buffer + 1, ']');
            if (end != NULL) {
                *end = '\0';
                settings_trim(buffer + 1);
                snprintf(section, sizeof(section), "%s", buffer + 1);
            }
            continue;
        }

        // Find the key-value separator.
        sep = strchr(buffer, '=');
        if (sep != NULL) {
            // Split the string into key and value.
            *sep++ = '\0';

            // Trim whitespace from both key and value.
            settings_trim(buffer);
            settings_trim(sep);

            // Only store entries with a real key. Empty values are valid.
            if (buffer[0] != '\0') {
                settings_set_str_value_ex(settings,
                    section[0] != '\0' ? section : NULL,
                    buffer,
                    sep,
                    false,
                    false);
            }
        }
    }

    fclose(stream);
}

/**
 * Saves settings to the associated file.
 *
 * 0x438C20
 */
void settings_save(Settings* settings)
{
    FILE* stream;
    SettingsEntry* curr;
    bool wrote_flat_entries;

    // Ensure there is something worth saving.
    if (settings->entries == NULL) {
        return;
    }

    // Only save settings have been changed.
    if ((settings->flags & SETTINGS_CHANGED) == 0) {
        return;
    }

    stream = fopen(settings->path, "wt");
    if (stream == NULL) {
        // Something's wrong, this should not normally happen.
        return;
    }

    wrote_flat_entries = false;
    curr = settings->entries;
    while (curr != NULL) {
        if (curr->section == NULL) {
            fprintf(stream, "%s=%s\n", curr->key, curr->value);
            wrote_flat_entries = true;
        }
        curr = curr->next;
    }

    curr = settings->entries;
    while (curr != NULL) {
        if (curr->section != NULL && !settings_section_was_written(settings, curr)) {
            SettingsEntry* section_entry;

            if (wrote_flat_entries) {
                fprintf(stream, "\n");
            }

            fprintf(stream, "[%s]\n", curr->section);
            section_entry = settings->entries;
            while (section_entry != NULL) {
                if (settings_section_equals(section_entry->section, curr->section)) {
                    fprintf(stream, "%s=%s\n", section_entry->key, section_entry->value);
                }
                section_entry = section_entry->next;
            }

            wrote_flat_entries = true;
        }
        curr = curr->next;
    }

    fclose(stream);

    // FIX: Settings synchronized to disk, unset `SETTINGS_CHANGED`.
    settings->flags &= ~SETTINGS_CHANGED;
}

void settings_set_write_defaults(Settings* settings, bool enabled)
{
    if (enabled) {
        settings->flags |= SETTINGS_WRITE_DEFAULTS;
    } else {
        settings->flags &= ~SETTINGS_WRITE_DEFAULTS;
    }
}

/**
 * Registers a new setting with a given key and default value.
 *
 * An optional callback `value_changed_func` can be specified as part of
 * setting. This callback will be triggered whenever the value associated with
 * a given key is changed. It will not be fired upon registration. Care should
 * be taken when changing the settings from within this callback to avoid
 * creating infinite loops.
 *
 * 0x438C80
 */
void settings_register(Settings* settings, const char* key, const char* default_value, SettingsValueChangedFunc value_changed_func)
{
    settings_register_ex(settings, NULL, key, default_value, value_changed_func);
}

void settings_register_section(Settings* settings, const char* section, const char* key, const char* default_value, SettingsValueChangedFunc value_changed_func)
{
    settings_register_ex(settings, section, key, default_value, value_changed_func);
}

/**
 * Sets an integer value for a setting.
 *
 * 0x438CE0
 */
void settings_set_value(Settings* settings, const char* key, int value)
{
    char buffer[48];

    SDL_itoa(value, buffer, 10);
    settings_set_str_value(settings, key, buffer);
}

/**
 * Retrieves an integer value for a setting.
 *
 * Returns `0` if the key is not found.
 *
 * 0x438D10
 */
int settings_get_value(Settings* settings, const char* key)
{
    const char* str;

    str = settings_get_str_value(settings, key);
    if (str != NULL) {
        return atoi(str);
    } else {
        return 0;
    }
}

/**
 * Sets an ObjectID value for a setting.
 *
 * 0x438D40
 */
void settings_set_obj_value(Settings* settings, const char* key, ObjectID oid)
{
    char str[40];

    objid_id_to_str(str, oid);
    settings_set_str_value(settings, key, str);
}

/**
 * Retrieves an ObjectID value for a setting.
 *
 * Returns an ObjectID with type `OID_TYPE_NULL` if the key is not found.
 *
 * 0x438DA0
 */
ObjectID settings_get_obj_value(Settings* settings, const char* key)
{
    const char* str;
    ObjectID oid;

    str = settings_get_str_value(settings, key);
    if (str != NULL) {
        objid_id_from_str(&oid, str);
    } else {
        oid.type = OID_TYPE_NULL;
    }

    return oid;
}

/**
 * Sets a string value for a setting.
 *
 * 0x438DF0
 */
void settings_set_str_value(Settings* settings, const char* key, const char* value)
{
    settings_set_str_value_ex(settings, NULL, key, value, true, true);
}

void settings_set_str_value_section(Settings* settings, const char* section, const char* key, const char* value)
{
    settings_set_str_value_ex(settings, section, key, value, true, true);
}

/**
 * Retrieves a string value for a setting.
 *
 * Returns `NULL` if the key is not found.
 *
 * 0x438E90
 */
const char* settings_get_str_value(Settings* settings, const char* key)
{
    SettingsEntry* entry;

    entry = settings_find_ex(settings, NULL, key);
    if (entry != NULL) {
        return entry->value;
    } else {
        return NULL;
    }
}

const char* settings_get_str_value_section(Settings* settings, const char* section, const char* key)
{
    SettingsEntry* entry;

    entry = settings_find_ex(settings, section, key);
    if (entry != NULL) {
        return entry->value;
    } else {
        return NULL;
    }
}

/**
 * Internal helper function to find a settings entry by key.
 *
 * 0x438EB0
 */
SettingsEntry* settings_find_ex(Settings* settings, const char* section, const char* key)
{
    SettingsEntry* curr;

    curr = settings->entries;
    while (curr != NULL) {
        if (settings_section_equals(curr->section, section)
            && SDL_strcasecmp(curr->key, key) == 0) {
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

void settings_register_ex(Settings* settings, const char* section, const char* key, const char* default_value, SettingsValueChangedFunc value_changed_func)
{
    SettingsEntry* entry;

    entry = settings_find_ex(settings, section, key);
    if (entry != NULL) {
        entry->value_changed_func = value_changed_func;
    } else {
        entry = (SettingsEntry*)MALLOC(sizeof(*entry));
        entry->section = section != NULL && section[0] != '\0' ? STRDUP(section) : NULL;
        entry->key = STRDUP(key);
        entry->value = STRDUP(default_value);
        entry->value_changed_func = value_changed_func;
        entry->next = settings->entries;
        settings->entries = entry;

        if ((settings->flags & SETTINGS_WRITE_DEFAULTS) != 0) {
            settings->flags |= SETTINGS_CHANGED;
        }
    }
}

void settings_set_str_value_ex(Settings* settings, const char* section, const char* key, const char* value, bool mark_changed, bool notify)
{
    SettingsEntry* entry;

    if (mark_changed) {
        settings->flags |= SETTINGS_CHANGED;
    }

    entry = settings_find_ex(settings, section, key);
    if (entry == NULL) {
        settings_register_ex(settings, section, key, value, NULL);
    } else {
        if (strlen(value) > strlen(entry->value)) {
            FREE(entry->value);
            entry->value = STRDUP(value);
        } else {
            memcpy(entry->value, value, strlen(value) + 1);
        }

        if (notify && entry->value_changed_func != NULL) {
            entry->value_changed_func();
        }
    }
}

bool settings_section_equals(const char* lhs, const char* rhs)
{
    if (lhs == NULL || lhs[0] == '\0') {
        return rhs == NULL || rhs[0] == '\0';
    }

    if (rhs == NULL || rhs[0] == '\0') {
        return false;
    }

    return SDL_strcasecmp(lhs, rhs) == 0;
}

bool settings_section_was_written(Settings* settings, SettingsEntry* entry)
{
    SettingsEntry* curr;

    curr = settings->entries;
    while (curr != NULL && curr != entry) {
        if (settings_section_equals(curr->section, entry->section)) {
            return true;
        }

        curr = curr->next;
    }

    return false;
}

/**
 * Internal helper function to trim leading and trailing whitespace from a
 * string (in place).
 *
 * 0x438EF0
 */
void settings_trim(char* str)
{
    char* curr;
    size_t len;

    // Skip leading whitespace.
    curr = str;
    while (SDL_isspace(*curr)) {
        curr++;
    }

    // Move the trimmed string to the start.
    len = strlen(curr);
    memmove(str, curr, len + 1); // FIX: Instead of `memcpy`.

    // Remove trailing whitespace.
    if (len > 0) {
        curr = str + len - 1;
        while (curr >= str && SDL_isspace(*curr)) {
            *curr-- = '\0';
        }
    }
}
