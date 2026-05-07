#include "game/settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/obj_private.h"

static SettingsEntry* settings_find(Settings* settings, const char* key);
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
    bool exists;
    TigFindFileData find_file_data;
    FILE* stream;
    char buffer[256];
    char* sep;

    // Check if the settings file exists.
    exists = tig_find_first_file(settings->path, &find_file_data);
    tig_find_close(&find_file_data); // FIX: Memory leak.

    if (!exists) {
        return;
    }

    stream = fopen(settings->path, "rt");
    if (stream == NULL) {
        return;
    }

    while (fgets(buffer, sizeof(buffer), stream) != NULL) {
        // Find the key-value separator.
        sep = strchr(buffer, '=');
        if (sep != NULL) {
            // Split the string into key and value.
            *sep++ = '\0';

            // Trim whitespace from both key and value.
            settings_trim(buffer);
            settings_trim(sep);

            // Only store non-empty key-value pairs.
            if (sep[0] != '\0' && buffer[0] != '\0') {
                settings_set_str_value(settings, buffer, sep);
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

    curr = settings->entries;
    while (curr != NULL) {
        fprintf(stream, "%s=%s\n", curr->key, curr->value);
        curr = curr->next;
    }

    fclose(stream);

    // FIX: Settings synchronized to disk, unset `SETTINGS_CHANGED`.
    settings->flags &= ~SETTINGS_CHANGED;
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
    SettingsEntry* entry;

    entry = settings_find(settings, key);
    if (entry != NULL) {
        entry->value_changed_func = value_changed_func;
    } else {
        entry = (SettingsEntry*)MALLOC(sizeof(*entry));
        entry->key = STRDUP(key);
        entry->value = STRDUP(default_value);
        entry->value_changed_func = value_changed_func;
        entry->next = settings->entries;
        settings->entries = entry;
    }
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
    SettingsEntry* entry;

    // Mark settings as changed to trigger saving.
    settings->flags |= SETTINGS_CHANGED;

    // Check if the key already exists.
    entry = settings_find(settings, key);
    if (entry == NULL) {
        // Add a new entry if the key does not exist.
        settings_register(settings, key, value, NULL);
    } else {
        // Update the existing entry's value.
        if (strlen(value) > strlen(entry->value)) {
            // The new string is longer than existing one - free existing value
            // and make a copy of the new string.
            FREE(entry->value);
            entry->value = STRDUP(value);
        } else {
            // Value fits into already existing buffer.
            memcpy(entry->value, value, strlen(value) + 1);
        }

        // Trigger the callback.
        if (entry->value_changed_func != NULL) {
            entry->value_changed_func();
        }
    }
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

    entry = settings_find(settings, key);
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
SettingsEntry* settings_find(Settings* settings, const char* key)
{
    SettingsEntry* curr;

    curr = settings->entries;
    while (curr != NULL) {
        if (SDL_strcasecmp(curr->key, key) == 0) {
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
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
        curr = str + len;
        while (SDL_isspace(*curr)) {
            *curr-- = '\0';
        }
    }
}
