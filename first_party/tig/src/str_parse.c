#include "tig/str_parse.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "tig/debug.h"

#define DEFAULT_LIST_SEPARATOR ','

// 0x5C22AC
static char tig_str_parse_separator = DEFAULT_LIST_SEPARATOR;

// 0x5318C0
int tig_str_parse_init(TigInitInfo* init_info)
{
    (void)init_info;

    tig_str_parse_separator = DEFAULT_LIST_SEPARATOR;

    return TIG_OK;
}

// 0x5318D0
void tig_str_parse_exit()
{
}

// 0x5318E0
void tig_str_parse_set_separator(int sep)
{
    tig_str_parse_separator = (unsigned char)sep;
}

// 0x5318F0
void tig_str_parse_str_value(char** str, char* value)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    memcpy(value, *str, strlen(*str) + 1);

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531990
void tig_str_parse_value(char** str, int* value)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    *value = atoi(*str);

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531A20
void tig_str_parse_value_64(char** str, int64_t* value)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    *value = SDL_strtoll(*str, NULL, 10);

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531AB0
void tig_str_parse_range(char** str, int* start, int* end)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    char* hyphen = strchr(*str, '-');
    if (hyphen != NULL) {
        *hyphen = '\0';
    }

    *start = atoi(*str);

    if (hyphen != NULL) {
        *hyphen = '-';
        *end = atoi(hyphen + 1);
    } else {
        *end = *start;
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531B70
void tig_str_parse_complex_value(char** str, int delim, int* value1, int* value2)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    if (*(*str) != '(') {
        tig_debug_printf("Tig StrParse: tig_str_parse_complex_value_value: ERROR: %s\n", *str);
        exit(EXIT_FAILURE);
    }

    (*str)++;

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    char* pch = strchr(*str, delim);
    if (pch != NULL) {
        *pch = '\0';
    }

    *value1 = atoi(*str);

    if (pch != NULL) {
        *pch = (unsigned char)delim;
        *value2 = atoi(pch + 1);
    } else {
        *value2 = *value1;
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531C60
void tig_str_parse_complex_value3(char** str, int delim, int* value1, int* value2, int* value3)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    if (*(*str) != '(') {
        tig_debug_printf("Tig StrParse: tig_str_parse_complex_value_value3: ERROR: %s\n", *str);
        exit(EXIT_FAILURE);
    }

    (*str)++;

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    char* pch = strchr(*str, delim);
    if (pch != NULL) {
        *pch = '\0';
    }

    *value1 = atoi(*str);

    if (pch != NULL) {
        *pch = (unsigned char)delim;
        *value2 = atoi(pch + 1);

        pch = strchr(pch + 1, delim);
        if (pch != NULL) {
            *value3 = atoi(pch + 1);
        } else {
            *value3 = *value2;
        }
    } else {
        *value2 = *value1;
        *value3 = *value1;
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531D80
void tig_str_parse_complex_str_value(char** str, int delim, const char** list, int list_length, int* value1, int* value2)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    if (*(*str) != '(') {
        tig_debug_printf("Tig StrParse: tig_str_parse_complex_str_value: ERROR: %s\n", *str);
        exit(EXIT_FAILURE);
    }

    (*str)++;

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    char* pch = strchr(*str, delim);
    if (pch != NULL) {
        *pch = '\0';
    }

    char* space = strchr(*str, ' ');
    if (space != NULL) {
        *space = '\0';
    }

    int index;
    for (index = 0; index < list_length; index++) {
        // NOTE: This approach is slightly wrong. When the entry is malformed
        // (identifier without value, i.e. "(foo)"), this approach consider
        // closing paren to be part of the identifier.
        if (SDL_strcasecmp(list[index], *str) == 0) {
            *value1 = index;
            break;
        }
    }

    if (space != NULL) {
        *space = ' ';
    }

    if (index >= list_length) {
        tig_debug_printf("Tig StrParse: tig_str_parse_complex_str_value: ERROR: %s\n", *str);
        exit(EXIT_FAILURE);
    }

    if (pch != NULL) {
        *pch = (unsigned char)delim;
        *value2 = atoi(pch + 1);
    } else {
        *value2 = *value1;
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531EE0
void tig_str_match_str_to_list(char** str, const char** list, int list_length, int* value)
{
    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    int index;
    for (index = 0; index < list_length; index++) {
        if (SDL_strcasecmp(list[index], *str) == 0) {
            *value = index;
            break;
        }
    }

    if (index >= list_length) {
        tig_debug_printf("Tig StrParse: tig_str_match_str_to_list: ERROR: %s\n", *str);
        exit(EXIT_FAILURE);
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531FB0
void tig_str_parse_flag_list(char** str, const char** keys, const unsigned int* values, int length, unsigned int* value)
{
    *value = 0;

    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    while (*str != NULL) {
        char* space = strchr(*str, ' ');
        char* pipe = strchr(*str, '|');
        if (space != NULL) {
            if (pipe == NULL) {
                *space = '\0';
            } else if (space < pipe) {
                *space = '\0';
            } else {
                space = NULL;
            }
        } else {
            if (pipe != NULL) {
                *pipe = '\0';
            }
        }

        int index;
        for (index = 0; index < length; index++) {
            if (SDL_strcasecmp(keys[index], *str) == 0) {
                *value |= values[index];
                break;
            }
        }

        if (index >= length) {
            tig_debug_printf("Tig StrParse: tig_str_parse_flag_list: ERROR: %s\n", *str);
            exit(EXIT_FAILURE);
        }

        if (pipe != NULL) {
            // FIXME: Why not pipe?
            *pipe = ' ';
            *str = pipe + 1;
        } else if (space != NULL) {
            *space = ' ';
            *str = space + 1;
        } else {
            *str = NULL;
        }

        if (*str != NULL) {
            if (*(*str) == '\0') {
                *str = NULL;
            }

            while (isspace(*(*str))) {
                (*str)++;
            }
        }
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x531FB0
void tig_str_parse_flag_list_64(char** str, const char** keys, const uint64_t* values, int length, uint64_t* value)
{
    *value = 0;

    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    while (*str != NULL) {
        char* space = strchr(*str, ' ');
        char* pipe = strchr(*str, '|');
        if (space != NULL) {
            if (pipe == NULL) {
                *space = '\0';
            } else if (space < pipe) {
                *space = '\0';
            } else {
                space = NULL;
            }
        } else {
            if (pipe != NULL) {
                *pipe = '\0';
            }
        }

        int index;
        for (index = 0; index < length; index++) {
            if (SDL_strcasecmp(keys[index], *str) == 0) {
                *value |= values[index];
                break;
            }
        }

        if (index >= length) {
            tig_debug_printf("Tig StrParse: tig_str_parse_flag_list_64: ERROR: String Match Failure: %s\n", *str);
            exit(EXIT_FAILURE);
        }

        if (pipe != NULL) {
            // FIXME: Why not pipe?
            *pipe = ' ';
            *str = pipe + 1;
        } else if (space != NULL) {
            *space = ' ';
            *str = space + 1;
        } else {
            *str = NULL;
        }

        if (*str != NULL) {
            if (*(*str) == '\0') {
                *str = NULL;
            }

            while (isspace(*(*str))) {
                (*str)++;
            }
        }
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}

// 0x532320
bool tig_str_parse_named_value(char** str, const char* name, int* value)
{
    *value = 0;

    if (*str == NULL) {
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        return false;
    }

    *str += strlen(name);

    tig_str_parse_value(str, value);

    return true;
}

// 0x5323B0
bool tig_str_parse_named_str_value(char** str, const char* name, char* value)
{
    if (*str == NULL) {
        value[0] = '\0';
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        value[0] = '\0';
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_parse_str_value(str, value);

    return true;
}

// 0x532480
bool tig_str_match_named_str_to_list(char** str, const char* name, const char** list, int list_length, int* value)
{
    if (*str == NULL) {
        *value = 0;
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        *value = 0;
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_match_str_to_list(str, list, list_length, value);

    return true;
}

// 0x532550
bool tig_str_parse_named_range(char** str, const char* name, int* start, int* end)
{
    if (*str == NULL) {
        *start = 0;
        *end = 0;
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        *start = 0;
        *end = 0;
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_parse_range(str, start, end);

    return true;
}

// 0x532630
bool tig_str_parse_named_flag_list(char** str, const char* name, const char** keys, const unsigned int* values, int length, unsigned int* value)
{
    *value = 0;

    if (*str == NULL) {
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_parse_flag_list(str, keys, values, length, value);

    return true;
}

// 0x532710
bool tig_str_parse_named_complex_value(char** str, const char* name, int delim, int* value1, int* value2)
{
    if (*str == NULL) {
        *value1 = 0;
        *value2 = 0;
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        *value1 = 0;
        *value2 = 0;
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_parse_complex_value(str, delim, value1, value2);

    return true;
}

// 0x532800
bool tig_str_parse_named_complex_value3(char** str, const char* name, int delim, int* value1, int* value2, int* value3)
{
    if (*str == NULL) {
        *value1 = 0;
        *value2 = 0;
        *value3 = 0;
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        *value1 = 0;
        *value2 = 0;
        *value3 = 0;
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_parse_complex_value3(str, delim, value1, value2, value3);

    return true;
}

// 0x5328F0
bool tig_str_parse_named_complex_str_value(char** str, const char* name, int delim, const char** list, int list_length, int* value1, int* value2)
{
    if (*str == NULL) {
        *value1 = 0;
        *value2 = 0;
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        *value1 = 0;
        *value2 = 0;
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_parse_complex_str_value(str, delim, list, list_length, value1, value2);

    return true;
}

// 0x5329E0
bool tig_str_parse_named_flag_list_64(char** str, const char* name, const char** keys, uint64_t* values, int length, uint64_t* value)
{
    *value = 0;

    if (*str == NULL) {
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        return false;
    }

    *str += strlen(name);

    while (isspace(*(*str))) {
        (*str)++;
    }

    tig_str_parse_flag_list_64(str, keys, values, length, value);

    return true;
}

// 0x532AC0
bool tig_str_parse_named_flag_list_direct(char** str, const char* name, const char** keys, int length, unsigned int* value)
{
    *value = 0;

    if (*str == NULL) {
        return false;
    }

    while (isspace(*(*str))) {
        (*str)++;
    }

    if (strstr(*str, name) != *str) {
        return false;
    }

    *str += strlen(name);

    tig_str_parse_flag_list_direct(str, keys, length, value);

    return true;
}

// 0x532B90
void tig_str_parse_flag_list_direct(char** str, const char** keys, int length, unsigned int* value)
{
    *value = 0;

    while (isspace(*(*str))) {
        (*str)++;
    }

    char* sep = strchr(*str, tig_str_parse_separator);
    if (sep != NULL) {
        *sep = '\0';
    }

    while (*str != NULL) {
        char* space = strchr(*str, ' ');
        char* pipe = strchr(*str, '|');
        if (space != NULL) {
            if (pipe == NULL) {
                *space = '\0';
            } else if (space < pipe) {
                *space = '\0';
            } else {
                space = NULL;
            }
        } else {
            if (pipe != NULL) {
                *pipe = '\0';
            }
        }

        int index;
        for (index = 0; index < length; index++) {
            if (SDL_strcasecmp(keys[index], *str) == 0) {
                *value |= 1 << index;
                break;
            }
        }

        if (index >= length) {
            tig_debug_printf("Tig StrParse: tig_str_parse_flag_list: ERROR: %s\n", *str);
            exit(EXIT_FAILURE);
        }

        if (pipe != NULL) {
            // FIXME: Why not pipe?
            *pipe = ' ';
            *str = pipe + 1;
        } else if (space != NULL) {
            *space = ' ';
            *str = space + 1;
        } else {
            *str = NULL;
        }

        if (*str != NULL) {
            if (*(*str) == '\0') {
                *str = NULL;
            }

            while (isspace(*(*str))) {
                (*str)++;
            }
        }
    }

    if (sep != NULL) {
        *sep = tig_str_parse_separator;
        *str = sep + 1;
    } else {
        *str = NULL;
    }
}
