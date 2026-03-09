#include "game/text_filter.h"

#include "game/critter.h"
#include "game/mes.h"

#define TEXT_FILTER_SLEEP_MES_FILE_NAME "mes\\text_filter_sleep.mes"

typedef bool(TextFilterValidate)(int64_t obj);
typedef bool(TextFilterInitFunc)(void);
typedef void(TextFilterExitFunc)(void);
typedef void(TextFilterProcess)(const char* src, char** dst);

typedef struct TextFilterInfo {
    const char* name;
    TextFilterValidate* validate;
    TextFilterInitFunc* init;
    TextFilterExitFunc* exit;
    TextFilterProcess* process;
} TextFilterInfo;

static bool text_filter_sleeping_init(void);
static void text_filter_sleeping_exit(void);
static bool text_filter_sleeping_validate(int64_t obj);
static void text_filter_sleeping_process(const char* src, char** dst);

// 0x5BCEB8
static TextFilterInfo stru_5BCEB8[] = {
    {
        "sleeping",
        text_filter_sleeping_validate,
        text_filter_sleeping_init,
        text_filter_sleeping_exit,
        text_filter_sleeping_process,
    },
    { 0 },
};

// 0x603DD4
static char** text_filter_sleeping_strings;

// 0x603DD8
static char* text_filter_sleeping_replacement_str;

// 0x603DDC
static size_t text_filter_sleeping_replacement_length;

// 0x4F67D0
bool text_filter_init(void)
{
    int index;

    for (index = 0; stru_5BCEB8[index].name != NULL; index++) {
        if (!stru_5BCEB8[index].init()) {
            tig_debug_printf("TextFilter: module %s(%d) failed initialization.\n", stru_5BCEB8[index].name, index);
            return false;
        }
    }

    return true;
}

// 0x4F6820
void text_filter_exit(void)
{
    int index;

    for (index = 0; stru_5BCEB8[index].name != NULL; index++) {
        stru_5BCEB8[index].exit();
    }
}

// 0x4F6840
bool text_filter_process(int64_t obj, const char* src, char** dst)
{
    int index;

    for (index = 0; stru_5BCEB8[index].name != NULL; index++) {
        if (stru_5BCEB8[index].validate(obj)) {
            stru_5BCEB8[index].process(src, dst);
            return true;
        }
    }

    *dst = STRDUP(src);
    return false;
}

// 0x4F6920
void sub_4F6920(void)
{
    // TODO: Incomplete.
}

// 0x4F69B0
void sub_4F69B0(void)
{
    // TODO: Incomplete.
}

// 0x4F69D0
bool text_filter_sleeping_init(void)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    int capacity = 10;

    if (!mes_load(TEXT_FILTER_SLEEP_MES_FILE_NAME, &mes_file)) {
        tig_debug_printf("text_filter_sleeping_init: ERROR: couldn't load message file: %s!\n", TEXT_FILTER_SLEEP_MES_FILE_NAME);
        exit(EXIT_SUCCESS); // FIXME: Should be `EXIT_FAILURE`.
    }

    text_filter_sleeping_strings = (char**)CALLOC(sizeof(*text_filter_sleeping_strings), capacity);

    mes_file_entry.num = 0;
    mes_get_msg(mes_file, &mes_file_entry);

    text_filter_sleeping_replacement_str = STRDUP(mes_file_entry.str);
    text_filter_sleeping_replacement_length = strlen(text_filter_sleeping_replacement_str);

    mes_file_entry.num = 1;
    while (mes_search(mes_file, &mes_file_entry)) {
        if (mes_file_entry.num > capacity - 1) {
            capacity *= 2;
            text_filter_sleeping_strings = (char**)REALLOC(text_filter_sleeping_strings, sizeof(*text_filter_sleeping_strings) * capacity);
        }

        mes_get_msg(mes_file, &mes_file_entry);
        text_filter_sleeping_strings[mes_file_entry.num - 1] = STRDUP(mes_file_entry.str);

        mes_file_entry.num++;
    }

    text_filter_sleeping_strings[mes_file_entry.num - 1] = NULL;

    if (!mes_unload(mes_file)) {
        tig_debug_printf("text_filter_sleeping_init: ERROR: couldn't unload message file: %s!\n", TEXT_FILTER_SLEEP_MES_FILE_NAME);
    }

    return true;
}

// 0x4F6B20
void text_filter_sleeping_exit(void)
{
    int index;

    for (index = 0; text_filter_sleeping_strings[index] != NULL; index++) {
        FREE(text_filter_sleeping_strings[index]);
    }
    FREE(text_filter_sleeping_strings);

    if (text_filter_sleeping_replacement_str != NULL) {
        FREE(text_filter_sleeping_replacement_str);
    }
}

// 0x4F6B70
bool text_filter_sleeping_validate(int64_t obj)
{
    return critter_is_unconscious(obj) || critter_is_sleeping(obj);
}

// 0x4F6BB0
void text_filter_sleeping_process(const char* src, char** dst)
{
    *dst = STRDUP(src);
    // TODO: Incomplete.
}
