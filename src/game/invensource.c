#include "game/invensource.h"

#include <stdio.h>

#include "game/mes.h"
#include "game/obj_private.h"
#include "game/proto.h"

#define MAX_INVEN_SOURCE_SET_NAME 70

typedef struct NamedInvenSourceSet {
    /* 0000 */ char name[MAX_INVEN_SOURCE_SET_NAME];
    /* 0048 */ InvenSourceSet set;
} NamedInvenSourceSet;

static bool parse_set_data(mes_file_handle_t invensource_mes_file, mes_file_handle_t invensourcebuy_mes_file);
static bool parse_invensource_entry(MesFileEntry* mes_file_entry, char* str);
static bool parse_invensourcebuy_entry(MesFileEntry* mes_file_entry, char* str);
static void show_error(const char* msg);

/**
 * Path to the inventory source message file.
 *
 * 0x5B64A8
 */
static const char* invensource_mes_file_name = "Rules\\InvenSource.mes";

/**
 * Path to the buy inventory source message file.
 *
 * 0x5B64AC
 */
static const char* invensourcebuy_mes_file_name = "Rules\\InvenSourceBuy.mes";

/**
 * Inventory source sets.
 *
 * 0x5FC520
 */
static NamedInvenSourceSet* invensource_sets;

/**
 * The number of items in `invensource_sets` array.
 *
 * 0x5FC524
 */
static int invensource_num_sets;

/**
 * Flag indicating whether the invensource system is in editor mode.
 *
 * 0x5FC528
 */
static bool invensource_editor;

/**
 * Buffer for storing error messages.
 *
 * 0x5FC52C
 */
static char invensource_error[256];

/**
 * 0x5FC62C
 */
static bool dword_5FC62C;

/**
 * Flag indicating whether the inventory source system is initialized.
 *
 * 0x5FC630
 */
static bool invensource_initialized;

/**
 * Flag indicating whether the invensourcebuy message file was loaded
 * successfully.
 *
 * 0x5FC634
 */
static bool invensource_have_buy;

/**
 * Called when the game is initialized.
 *
 * 0x4BF390
 */
bool invensource_init(GameInitInfo* init_info)
{
    mes_file_handle_t invensource_mes_file;
    mes_file_handle_t invensourcebuy_mes_file;

    if (!invensource_initialized) {
        invensource_editor = init_info->editor;

        // Load inventory source message file (required).
        if (!mes_load(invensource_mes_file_name, &invensource_mes_file)) {
            sprintf(invensource_error, "Can't load message file [%s].", invensource_mes_file_name);
            show_error(invensource_error);
            return false;
        }

        // Load buy inventory source message file (optional, but triggers error
        // message in the editor).
        if (mes_load(invensourcebuy_mes_file_name, &invensourcebuy_mes_file)) {
            invensource_have_buy = true;
        } else {
            invensource_have_buy = false;
            sprintf(invensource_error, "Can't load message file [%s].", invensourcebuy_mes_file_name);
            show_error(invensource_error);
        }

        // Retrieve the number of entries in the primary message file.
        invensource_num_sets = mes_entries_count(invensource_mes_file);
        if (invensource_num_sets < 1) {
            sprintf(invensource_error, "No sets to parse in [%s].", invensource_mes_file_name);
            mes_unload(invensource_mes_file);
            mes_unload(invensourcebuy_mes_file);
            return false;
        }

        invensource_sets = MALLOC(sizeof(*invensource_sets) * invensource_num_sets);

        // Parse the message files into the sets array.
        if (!parse_set_data(invensource_mes_file, invensourcebuy_mes_file)) {
            FREE(invensource_sets); // FIX: Memory leak.
            mes_unload(invensource_mes_file);
            mes_unload(invensourcebuy_mes_file);
            return false;
        }

        // Clean up parser state.
        mes_unload(invensource_mes_file);
        mes_unload(invensourcebuy_mes_file);
        invensource_have_buy = false;

        invensource_initialized = true;
    }

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4BF510
 */
void invensource_exit(void)
{
    if (invensource_initialized) {
        FREE(invensource_sets);
        invensource_initialized = false;
    }
}

/**
 * Retrieves an inventory source set by ID.
 *
 * 0x4BF5D0
 */
void invensource_get_id_list(int id, InvenSourceSet* set)
{
    if (invensource_initialized) {
        // TODO: Since `id` is 1-based, `id < invensource_num_sets` looks wrong.
        if (id > 0 && id < invensource_num_sets) {
            *set = invensource_sets[id - 1].set;
            return;
        }

        tig_debug_printf("Range error in invensource_get_id_list, set: %d\n", id);
    }

    // Return an empty set for invalid cases.
    set->cnt = 0;
    set->buy_cnt = 0;
}

/**
 * Parses inventory source data from message files into the global sets array.
 *
 * 0x4BF640
 */
bool parse_set_data(mes_file_handle_t invensource_mes_file, mes_file_handle_t invensourcebuy_mes_file)
{
    MesFileEntry mes_file_entry1;
    MesFileEntry mes_file_entry2;
    char str[MAX_STRING];
    int index;
    NamedInvenSourceSet* named_set;

    index = 0;
    mes_file_entry1.num = 0;

    // Iterate through all entries in the primary message file.
    while (mes_find_next(invensource_mes_file, &mes_file_entry1)) {
        // Check for discontinuous entry numbers.
        if (mes_file_entry1.num != ++index) {
            sprintf(invensource_error,
                "[%s] discontinuous at line: %d.  Skipping remaining sets.",
                invensource_mes_file_name,
                mes_file_entry1.num);
            show_error(invensource_error);

            // As the error above states, the remaining sets are skipped.
            return true;
        }

        named_set = &(invensource_sets[mes_file_entry1.num - 1]);

        // Parse the primary inventory source data.
        if (!parse_invensource_entry(&mes_file_entry1, str)) {
            sprintf(invensource_error,
                "Error parsing [%s] line %d.  Emptying set.",
                invensource_mes_file_name,
                mes_file_entry1.num);
            show_error(invensource_error);
            named_set->set.cnt = 0;
            named_set->set.buy_cnt = 0;

            continue;
        }

        // Parse buy data if available.
        named_set->set.buy_cnt = 0;
        if (invensource_have_buy) {
            mes_file_entry2.num = mes_file_entry1.num;
            if (mes_search(invensourcebuy_mes_file, &mes_file_entry2)) {
                mes_get_msg(invensourcebuy_mes_file, &mes_file_entry2);
                if (!parse_invensourcebuy_entry(&mes_file_entry2, str)) {
                    sprintf(invensource_error,
                        "Error parsing [%s] line %d.  Emptying set.",
                        invensourcebuy_mes_file_name,
                        mes_file_entry1.num);
                    show_error(invensource_error);

                    named_set->set.cnt = 0;
                    named_set->set.buy_cnt = 0;
                }
            }
        }
    }

    if (index != invensource_num_sets) {
        show_error("Set count is off in InvenSource, parse_set_data.");
    }

    return true;
}

/**
 * Parses a single inventory source set from a message file entry.
 *
 * 0x4BF7B0
 */
bool parse_invensource_entry(MesFileEntry* mes_file_entry, char* str)
{
    NamedInvenSourceSet* named_set;
    char* tok;
    int cnt;
    int rate;
    int basic_prototype;
    bool is_first;
    int64_t obj;

    // Check if the entry string exceeds maximum length.
    if (strlen(mes_file_entry->str) >= MAX_STRING) {
        sprintf(invensource_error,
            "Line %d in [%s] exceeds the maximum line length for message files.",
            mes_file_entry->num,
            invensource_mes_file_name);
        show_error(invensource_error);
        return false;
    }

    // Copy the entry string to the working buffer.
    strcpy(str, mes_file_entry->str);
    named_set = &(invensource_sets[mes_file_entry->num - 1]);

    // Parse the set name (before the colon).
    tok = strtok(str, ":");
    if (tok == NULL) {
        sprintf(invensource_error, "Line %d in [%s] is empty.\n", mes_file_entry->num, invensource_mes_file_name);
        show_error(invensource_error);
        return false;
    }

    // Validate the set name length.
    if (strlen(tok) >= MAX_INVEN_SOURCE_SET_NAME) {
        sprintf(invensource_error,
            "Line %d in [%s] has too long of a name.\n  Name length should be kept under %d characters.\n",
            mes_file_entry->num,
            invensource_mes_file_name,
            MAX_INVEN_SOURCE_SET_NAME);
        show_error(invensource_error);
        return false;
    }

    strcpy(named_set->name, tok);

    // Parse the first pair.
    tok = strtok(NULL, ",");
    if (tok == NULL) {
        // No data - this should not normally happen, inven source is probably
        // malformed.
        named_set->set.cnt = 0;
        named_set->set.min_coins = 0;
        named_set->set.max_coins = 0;
        return true;
    }

    cnt = 0;
    is_first = true;
    while (tok != NULL) {
        // Check for item count exceeding maximum.
        if (cnt >= INVEN_SOURCE_SET_SIZE) {
            sprintf(invensource_error,
                "Line %d in [%s] has too many items.\n  Item total should not exceed %d.\n",
                mes_file_entry->num,
                invensource_mes_file_name,
                INVEN_SOURCE_SET_SIZE);
            show_error(invensource_error);
            return false;
        }

        rate = atoi(tok);

        tok = strtok(NULL, " ");
        if (tok == NULL) {
            sprintf(invensource_error,
                "Error: Line %d in [%s] has an incomplete rate,bp pair at entry %d.\n",
                mes_file_entry->num,
                invensource_mes_file_name,
                cnt + 1);
            show_error(invensource_error);
            return false;
        }

        basic_prototype = atoi(tok);

        if (is_first) {
            // The first pair is min/max coins to generate.
            named_set->set.min_coins = rate;
            named_set->set.max_coins = basic_prototype;
            is_first = false;
        } else {
            // Validate basic prototype.
            if (!proto_is_valid(basic_prototype)) {
                sprintf(invensource_error,
                    "Error: Invalid prototype in [%s]: set %d, entry %d, basic prototype %d\n",
                    invensource_mes_file_name,
                    mes_file_entry->num,
                    cnt + 1,
                    basic_prototype);
                show_error(invensource_error);
                return false;
            }

            // Validate prototype object.
            obj = sub_4685A0(basic_prototype);
            if (!obj_handle_is_valid(obj)) {
                sprintf(invensource_error,
                    "Error: Can't get valid handle for prototype in [%s]: set %d, entry %d, basic prototype %d\n",
                    invensource_mes_file_name,
                    mes_file_entry->num,
                    cnt + 1,
                    basic_prototype);
                show_error(invensource_error);
                return false;
            }

            named_set->set.rate[cnt] = rate;
            named_set->set.basic_prototype[cnt] = basic_prototype;
            cnt++;
        }

        tok = strtok(NULL, ",");
    }

    named_set->set.cnt = cnt;
    return true;
}

/**
 * Parses a single buy inventory source set from a message file entry.
 *
 * 0x4BFAA0
 */
bool parse_invensourcebuy_entry(MesFileEntry* mes_file_entry, char* str)
{
    NamedInvenSourceSet* named_set;
    char* tok;
    int cnt;
    int basic_prototype;
    int64_t obj;

    // Check if the entry string exceeds maximum length.
    if (strlen(mes_file_entry->str) >= MAX_STRING) {
        sprintf(invensource_error,
            "Line %d in [%s] exceeds the maximum line length for message files.",
            mes_file_entry->num,
            invensourcebuy_mes_file_name);
        show_error(invensource_error);
        return false;
    }

    // Copy the entry string to the working buffer.
    strcpy(str, mes_file_entry->str);
    named_set = &(invensource_sets[mes_file_entry->num - 1]);

    // Parse the first token.
    tok = strtok(str, " ");
    if (tok == NULL) {
        // No data - this inven source won't buy anything.
        named_set->set.buy_all = false;
        named_set->set.buy_cnt = 0;
        return true;
    }

    // Special case - "ALL" keyword enables buying all items.
    if (strcmp(SDL_strupr(tok), "ALL") == 0) {
        named_set->set.buy_all = true;
        named_set->set.buy_cnt = 0;
        return true;
    }

    named_set->set.buy_all = false;

    // Parse space-separated basic prototypes.
    cnt = 0;
    while (tok != NULL) {
        // Check for item count exceeding maximum.
        if (cnt >= INVEN_SOURCE_SET_SIZE) {
            sprintf(invensource_error,
                "Line %d in [%s] has too many items.\n  Item total should not exceed %d.\n",
                mes_file_entry->num,
                invensourcebuy_mes_file_name,
                INVEN_SOURCE_SET_SIZE);
            show_error(invensource_error);
            return false;
        }

        basic_prototype = atoi(tok);

        // Validate basic prototype.
        if (!proto_is_valid(basic_prototype)) {
            sprintf(invensource_error,
                "Error: Invalid prototype in [%s]: set %d, entry %d, basic prototype %d\n",
                invensourcebuy_mes_file_name,
                mes_file_entry->num,
                cnt + 1,
                basic_prototype);
            show_error(invensource_error);
            return false;
        }

        // Validate prototype object.
        obj = sub_4685A0(basic_prototype);
        if (!obj_handle_is_valid(obj)) {
            sprintf(invensource_error,
                "Error: Can't get valid handle for prototype in [%s]: set %d, entry %d, basic prototype %d\n",
                invensourcebuy_mes_file_name,
                mes_file_entry->num,
                cnt + 1,
                basic_prototype);
            show_error(invensource_error);
            return false;
        }

        named_set->set.buy_basic_prototype[cnt++] = basic_prototype;

        tok = strtok(NULL, " ");
    }

    named_set->set.buy_cnt = cnt;
    return true;
}

/**
 * Logs (and displays - in the editor mode) an error message.
 *
 * 0x4BFCA0
 */
void show_error(const char* msg)
{
    tig_debug_println(msg);

    if (!dword_5FC62C && invensource_editor) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", msg, NULL);
    }
}
