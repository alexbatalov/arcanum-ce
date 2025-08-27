#ifndef TIG_STR_PARSE_H_
#define TIG_STR_PARSE_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

int tig_str_parse_init(TigInitInfo* init_info);
void tig_str_parse_exit();
void tig_str_parse_set_separator(int sep);
void tig_str_parse_str_value(char** str, char* value);
void tig_str_parse_value(char** str, int* value);
void tig_str_parse_value_64(char** str, int64_t* value);
void tig_str_parse_range(char** str, int* start, int* end);
void tig_str_parse_complex_value(char** str, int delim, int* value1, int* value2);
void tig_str_parse_complex_value3(char** str, int delim, int* value1, int* value2, int* value3);
void tig_str_parse_complex_str_value(char** str, int delim, const char** list, int list_length, int* value1, int* value2);
void tig_str_match_str_to_list(char** str, const char** list, int list_length, int* value);
void tig_str_parse_flag_list(char** str, const char** keys, const unsigned int* values, int length, unsigned int* value);
void tig_str_parse_flag_list_64(char** str, const char** keys, const uint64_t* values, int length, uint64_t* value);
bool tig_str_parse_named_value(char** str, const char* name, int* value);
bool tig_str_parse_named_str_value(char** str, const char* name, char* value);
bool tig_str_match_named_str_to_list(char** str, const char* name, const char** list, int list_length, int* value);
bool tig_str_parse_named_range(char** str, const char* name, int* start, int* end);
bool tig_str_parse_named_flag_list(char** str, const char* name, const char** keys, const unsigned int* values, int length, unsigned int* value);
bool tig_str_parse_named_complex_value(char** str, const char* name, int delim, int* value1, int* value2);
bool tig_str_parse_named_complex_value3(char** str, const char* name, int delim, int* value1, int* value2, int* value3);
bool tig_str_parse_named_complex_str_value(char** str, const char* name, int delim, const char** list, int list_length, int* value1, int* value2);
bool tig_str_parse_named_flag_list_64(char** str, const char* name, const char** keys, uint64_t* values, int length, uint64_t* value);
bool tig_str_parse_named_flag_list_direct(char** str, const char* name, const char** keys, int length, unsigned int* value);
void tig_str_parse_flag_list_direct(char** str, const char** keys, int length, unsigned int* value);

#ifdef __cplusplus
}
#endif

#endif /* TIG_STR_PARSE_H_ */
