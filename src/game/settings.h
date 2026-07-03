#ifndef ARCANUM_GAME_SETTINGS_H_
#define ARCANUM_GAME_SETTINGS_H_

#include <stdbool.h>

#include "game/obj.h"

typedef unsigned int SettingsFlags;

#define SETTINGS_CHANGED 0x1
#define SETTINGS_WRITE_DEFAULTS 0x2

typedef void (*SettingsValueChangedFunc)(void);

typedef struct SettingsEntry {
    /* 0000 */ char* section;
    /* 0004 */ char* key;
    /* 0008 */ char* value;
    /* 000C */ SettingsValueChangedFunc value_changed_func;
    /* 0010 */ struct SettingsEntry* next;
} SettingsEntry;

typedef struct Settings {
    /* 0000 */ const char* path;
    /* 0004 */ SettingsEntry* entries;
    /* 0008 */ SettingsFlags flags;
} Settings;

void settings_init(Settings* settings, const char* path);
void settings_exit(Settings* settings);
void settings_load(Settings* settings);
void settings_save(Settings* settings);
void settings_set_write_defaults(Settings* settings, bool enabled);
void settings_register(Settings* settings, const char* key, const char* default_value, SettingsValueChangedFunc value_changed_func);
void settings_register_section(Settings* settings, const char* section, const char* key, const char* default_value, SettingsValueChangedFunc value_changed_func);
void settings_set_value(Settings* settings, const char* key, int value);
int settings_get_value(Settings* settings, const char* key);
void settings_set_obj_value(Settings* settings, const char* key, ObjectID oid);
ObjectID settings_get_obj_value(Settings* settings, const char* key);
void settings_set_str_value(Settings* settings, const char* key, const char* value);
void settings_set_str_value_section(Settings* settings, const char* section, const char* key, const char* value);
const char* settings_get_str_value(Settings* settings, const char* key);
const char* settings_get_str_value_section(Settings* settings, const char* section, const char* key);

#endif /* ARCANUM_GAME_SETTINGS_H_ */
