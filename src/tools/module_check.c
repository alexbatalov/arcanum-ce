#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define SCRIPT_HEADER_SIZE 8
#define SCRIPT_CODE_HEADER_SIZE 56
#define SCRIPT_ENTRY_OFFSET (SCRIPT_HEADER_SIZE + SCRIPT_CODE_HEADER_SIZE)
#define SCRIPT_CONDITION_SIZE 0x84
#define SCRIPT_CONDITION_ACTION_OFFSET 0x2C
#define SCRIPT_CONDITION_ELSE_OFFSET 0x58
#define SCRIPT_CONDITION_TYPE_COUNT 53
#define SCRIPT_ACTION_TYPE_COUNT 142

#define DIALOG_FIELD_SIZE 1000
#define MES_FIELD_SIZE 2000
#define PROFILE_CHECK_MAX_PATH 260
#define PROFILE_CHECK_MAX_EXPECTED_PATHS 64

typedef enum FileKind {
    FILE_KIND_SCRIPT,
    FILE_KIND_DIALOG,
    FILE_KIND_MESSAGE,
    FILE_KIND_PROTOTYPE,
    FILE_KIND_MOBILE,
    FILE_KIND_SECTOR,
    FILE_KIND_MAP,
    FILE_KIND_ART,
    FILE_KIND_SOUND,
    FILE_KIND_DAT,
    FILE_KIND_OTHER,
    FILE_KIND_COUNT,
} FileKind;

typedef enum FieldResult {
    FIELD_RESULT_EOF,
    FIELD_RESULT_OK,
    FIELD_RESULT_ERROR,
} FieldResult;

typedef struct IntList {
    int* values;
    size_t length;
    size_t capacity;
} IntList;

typedef struct StringList {
    char** values;
    size_t length;
    size_t capacity;
} StringList;

typedef struct DatEntry {
    char* dat_path;
    char* entry_path;
} DatEntry;

typedef struct DatEntryList {
    DatEntry* values;
    size_t length;
    size_t capacity;
} DatEntryList;

typedef struct ProfileCheck {
    bool enabled;
    bool loaded;
    char path[PROFILE_CHECK_MAX_PATH];
    char module[PROFILE_CHECK_MAX_PATH];
    size_t expected_path_count;
    char expected_paths[PROFILE_CHECK_MAX_EXPECTED_PATHS][PROFILE_CHECK_MAX_PATH];
} ProfileCheck;

typedef struct DialogRecord {
    int num;
    int iq;
    int response_val;
    int line;
} DialogRecord;

typedef struct DialogRecordList {
    DialogRecord* values;
    size_t length;
    size_t capacity;
} DialogRecordList;

typedef struct Scanner {
    bool verbose;
    uint64_t directories_scanned;
    uint64_t files_scanned;
    uint64_t bytes_scanned;
    uint64_t file_kind_counts[FILE_KIND_COUNT];
    uint64_t issues;
    uint64_t scripts_parsed;
    uint64_t scripts_failed;
    uint64_t script_entries;
    uint64_t script_condition_counts[256];
    uint64_t script_action_counts[256];
    uint64_t script_condition_out_of_range;
    uint64_t script_action_out_of_range;
    uint64_t dialogs_parsed;
    uint64_t dialogs_failed;
    uint64_t dialog_entries;
    uint64_t messages_parsed;
    uint64_t messages_failed;
    uint64_t message_entries;
    uint64_t dats_indexed;
    uint64_t dats_failed;
    uint64_t dat_entries;
    IntList script_ids;
    IntList dialog_ids;
    StringList loose_paths;
    DatEntryList dat_index_entries;
    const char* root_path;
    size_t root_path_len;
} Scanner;

static void usage(void);
static int scan_path(Scanner* scanner, const char* path);
static int scan_directory(Scanner* scanner, const char* path);
static int scan_file(Scanner* scanner, const char* path, uint64_t size);
static int scan_script(Scanner* scanner, const char* path, uint64_t size);
static int scan_dialog(Scanner* scanner, const char* path);
static int scan_message(Scanner* scanner, const char* path);
static int scan_dat_archive(Scanner* scanner, const char* path);
static FieldResult read_braced_field(Scanner* scanner, FILE* stream, const char* path, char* buffer, size_t size, int* line_ptr, bool skip_line_comments);
static FileKind file_kind_from_path(const char* path);
static const char* file_kind_name(FileKind kind);
static const char* file_name(const char* path);
static const char* relative_path(const Scanner* scanner, const char* path);
static char* join_path(const char* base, const char* name);
static char* string_duplicate(const char* str);
static char* normalized_path_copy(const char* path);
static bool has_extension(const char* path, const char* extension);
static bool parse_int_field(const char* str, int* value_ptr);
static bool parse_leading_int(const char* str, int* value_ptr);
static int dialog_record_list_find(const DialogRecordList* list, int num);
static bool dialog_record_list_append(DialogRecordList* list, DialogRecord record);
static void dialog_record_list_free(DialogRecordList* list);
static bool int_list_contains(const IntList* list, int value);
static bool int_list_append(IntList* list, int value);
static void int_list_free(IntList* list);
static bool string_list_contains(const StringList* list, const char* value);
static bool string_list_append(StringList* list, char* value);
static void string_list_free(StringList* list);
static bool dat_entry_list_append(DatEntryList* list, const char* dat_path, char* entry_path);
static void dat_entry_list_free(DatEntryList* list);
static int read_i32_le(const unsigned char* bytes);
static bool read_i32(FILE* stream, int32_t* value_ptr);
static bool read_u32(FILE* stream, uint32_t* value_ptr);
static bool read_u32_le_at(FILE* stream, long offset, uint32_t* value_ptr);
static void print_dat_override_report(const Scanner* scanner);
static bool profile_check_load(Scanner* scanner, const char* path, ProfileCheck* profile);
static void profile_check_parse_line(ProfileCheck* profile, char* line);
static void profile_check_add_expected_path(ProfileCheck* profile, const char* path);
static void profile_check_apply(Scanner* scanner, const char* root_path, const ProfileCheck* profile);
static void profile_check_print(const ProfileCheck* profile);
static void profile_check_strip_comments(char* line);
static void profile_check_trim(char** start_ptr, char** end_ptr);
static bool profile_check_key_equals(const char* lhs, const char* rhs);
static bool profile_check_path_exists(const char* path);
static bool profile_check_is_absolute_or_relative(const char* path);
static void scanner_issue(Scanner* scanner, const char* path, const char* format, ...);
static void print_histogram(const char* title, const uint64_t* values, size_t count, size_t known_count);

#define FOURCC_DAT ((int32_t)(((uint32_t)' ') | ((uint32_t)'T' << 8) | ((uint32_t)'A' << 16) | ((uint32_t)'D' << 24)))
#define FOURCC_DAT1 ((int32_t)(((uint32_t)'1') | ((uint32_t)'T' << 8) | ((uint32_t)'A' << 16) | ((uint32_t)'D' << 24)))

int main(int argc, char** argv)
{
    Scanner scanner;
    ProfileCheck profile;
    const char* path = NULL;
    const char* profile_path = NULL;
    int arg;
    int rc;

    memset(&scanner, 0, sizeof(scanner));
    memset(&profile, 0, sizeof(profile));

    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            usage();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[arg], "--verbose") == 0 || strcmp(argv[arg], "-v") == 0) {
            scanner.verbose = true;
        } else if (strcmp(argv[arg], "--profile") == 0) {
            if (arg + 1 >= argc) {
                fprintf(stderr, "--profile requires a config path\n");
                usage();
                return EXIT_FAILURE;
            }
            profile_path = argv[++arg];
        } else if (path == NULL) {
            path = argv[arg];
        } else {
            fprintf(stderr, "Unexpected argument: %s\n", argv[arg]);
            usage();
            return EXIT_FAILURE;
        }
    }

    if (path == NULL) {
        usage();
        return EXIT_FAILURE;
    }

    if (profile_path != NULL) {
        profile.enabled = true;
        profile_check_load(&scanner, profile_path, &profile);
    }

    rc = scan_path(&scanner, path);
    profile_check_apply(&scanner, path, &profile);

    printf("Arcanum CE module check\n");
    printf("Path: %s\n", path);
    printf("Directories scanned: %" PRIu64 "\n", scanner.directories_scanned);
    printf("Files scanned: %" PRIu64 "\n", scanner.files_scanned);
    printf("Bytes scanned: %" PRIu64 "\n", scanner.bytes_scanned);
    printf("\nFile inventory:\n");
    for (FileKind kind = 0; kind < FILE_KIND_COUNT; kind++) {
        if (scanner.file_kind_counts[kind] != 0) {
            printf("  %s: %" PRIu64 "\n", file_kind_name(kind), scanner.file_kind_counts[kind]);
        }
    }

    printf("\nScripts:\n");
    printf("  parsed: %" PRIu64 "\n", scanner.scripts_parsed);
    printf("  failed: %" PRIu64 "\n", scanner.scripts_failed);
    printf("  entries: %" PRIu64 "\n", scanner.script_entries);
    printf("  condition ids outside ScriptConditionType: %" PRIu64 "\n", scanner.script_condition_out_of_range);
    printf("  action ids outside ScriptActionType: %" PRIu64 "\n", scanner.script_action_out_of_range);
    print_histogram("  condition ids", scanner.script_condition_counts, 256, SCRIPT_CONDITION_TYPE_COUNT);
    print_histogram("  action ids", scanner.script_action_counts, 256, SCRIPT_ACTION_TYPE_COUNT);

    printf("\nDialogs:\n");
    printf("  parsed: %" PRIu64 "\n", scanner.dialogs_parsed);
    printf("  failed: %" PRIu64 "\n", scanner.dialogs_failed);
    printf("  entries: %" PRIu64 "\n", scanner.dialog_entries);

    printf("\nMessages:\n");
    printf("  parsed: %" PRIu64 "\n", scanner.messages_parsed);
    printf("  failed: %" PRIu64 "\n", scanner.messages_failed);
    printf("  entries: %" PRIu64 "\n", scanner.message_entries);

    printf("\nDAT archives:\n");
    printf("  indexed: %" PRIu64 "\n", scanner.dats_indexed);
    printf("  failed: %" PRIu64 "\n", scanner.dats_failed);
    printf("  entries: %" PRIu64 "\n", scanner.dat_entries);
    print_dat_override_report(&scanner);
    profile_check_print(&profile);

    printf("\nIssues: %" PRIu64 "\n", scanner.issues);

    int_list_free(&scanner.script_ids);
    int_list_free(&scanner.dialog_ids);
    string_list_free(&scanner.loose_paths);
    dat_entry_list_free(&scanner.dat_index_entries);

    if (rc != 0 || scanner.issues != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void usage(void)
{
    printf("Usage: arcanum-ce-module-check [--verbose] [--profile <cfg>] <module-or-data-path>\n");
    printf("\n");
    printf("Recursively scans loose Arcanum files without launching the game or writing to the input path.\n");
}

int scan_path(Scanner* scanner, const char* path)
{
#ifdef _WIN32
    DWORD attributes;
    WIN32_FILE_ATTRIBUTE_DATA data;
    uint64_t size;

    attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        scanner_issue(scanner, path, "cannot stat path");
        return -1;
    }

    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        scanner->root_path = path;
        scanner->root_path_len = strlen(path);
        return scan_directory(scanner, path);
    }

    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        scanner_issue(scanner, path, "cannot stat file");
        return -1;
    }

    size = ((uint64_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
    return scan_file(scanner, path, size);
#else
    struct stat st;

    if (stat(path, &st) != 0) {
        scanner_issue(scanner, path, "cannot stat path");
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        scanner->root_path = path;
        scanner->root_path_len = strlen(path);
        return scan_directory(scanner, path);
    }

    return scan_file(scanner, path, (uint64_t)st.st_size);
#endif
}

int scan_directory(Scanner* scanner, const char* path)
{
    int rc = 0;

    scanner->directories_scanned++;

#ifdef _WIN32
    char* pattern;
    HANDLE handle;
    WIN32_FIND_DATAA data;

    pattern = join_path(path, "*");
    if (pattern == NULL) {
        scanner_issue(scanner, path, "out of memory while building search path");
        return -1;
    }

    handle = FindFirstFileA(pattern, &data);
    free(pattern);
    if (handle == INVALID_HANDLE_VALUE) {
        scanner_issue(scanner, path, "cannot enumerate directory");
        return -1;
    }

    do {
        char* child;
        uint64_t size;

        if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0) {
            continue;
        }

        child = join_path(path, data.cFileName);
        if (child == NULL) {
            scanner_issue(scanner, path, "out of memory while building child path");
            rc = -1;
            continue;
        }

        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            if (scan_directory(scanner, child) != 0) {
                rc = -1;
            }
        } else {
            size = ((uint64_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
            if (scan_file(scanner, child, size) != 0) {
                rc = -1;
            }
        }

        free(child);
    } while (FindNextFileA(handle, &data));

    FindClose(handle);
#else
    DIR* dir;
    struct dirent* entry;

    dir = opendir(path);
    if (dir == NULL) {
        scanner_issue(scanner, path, "cannot enumerate directory");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        char* child;
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        child = join_path(path, entry->d_name);
        if (child == NULL) {
            scanner_issue(scanner, path, "out of memory while building child path");
            rc = -1;
            continue;
        }

        if (stat(child, &st) != 0) {
            scanner_issue(scanner, child, "cannot stat directory entry");
            free(child);
            rc = -1;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (scan_directory(scanner, child) != 0) {
                rc = -1;
            }
        } else if (scan_file(scanner, child, (uint64_t)st.st_size) != 0) {
            rc = -1;
        }

        free(child);
    }

    closedir(dir);
#endif

    return rc;
}

int scan_file(Scanner* scanner, const char* path, uint64_t size)
{
    FileKind kind;
    int id;

    scanner->files_scanned++;
    scanner->bytes_scanned += size;

    kind = file_kind_from_path(path);
    scanner->file_kind_counts[kind]++;

    if (scanner->root_path != NULL && kind != FILE_KIND_DAT) {
        char* normalized = normalized_path_copy(relative_path(scanner, path));
        if (normalized == NULL || !string_list_append(&scanner->loose_paths, normalized)) {
            free(normalized);
            scanner_issue(scanner, path, "out of memory while recording loose file path");
            return -1;
        }
    }

    if (scanner->verbose) {
        printf("checking %s\n", path);
    }

    if (kind == FILE_KIND_SCRIPT && parse_leading_int(file_name(path), &id)) {
        if (int_list_contains(&scanner->script_ids, id)) {
            scanner_issue(scanner, path, "duplicate script id %d", id);
        } else if (!int_list_append(&scanner->script_ids, id)) {
            scanner_issue(scanner, path, "out of memory while recording script id");
            return -1;
        }
    } else if (kind == FILE_KIND_DIALOG && parse_leading_int(file_name(path), &id)) {
        if (int_list_contains(&scanner->dialog_ids, id)) {
            scanner_issue(scanner, path, "duplicate dialog id %d", id);
        } else if (!int_list_append(&scanner->dialog_ids, id)) {
            scanner_issue(scanner, path, "out of memory while recording dialog id");
            return -1;
        }
    }

    switch (kind) {
    case FILE_KIND_SCRIPT:
        return scan_script(scanner, path, size);
    case FILE_KIND_DIALOG:
        return scan_dialog(scanner, path);
    case FILE_KIND_MESSAGE:
        return scan_message(scanner, path);
    case FILE_KIND_DAT:
        return scan_dat_archive(scanner, path);
    default:
        return 0;
    }
}

int scan_script(Scanner* scanner, const char* path, uint64_t size)
{
    FILE* stream;
    uint32_t num_entries_raw;
    uint32_t max_entries_raw;
    uint64_t available_entries;
    uint64_t entries_to_read;
    uint64_t expected_size;
    unsigned char entry[SCRIPT_CONDITION_SIZE];
    uint64_t index;
    bool failed = false;

    if (size < SCRIPT_ENTRY_OFFSET) {
        scanner_issue(scanner, path, "script is too small: %" PRIu64 " bytes", size);
        scanner->scripts_failed++;
        return 0;
    }

    stream = fopen(path, "rb");
    if (stream == NULL) {
        scanner_issue(scanner, path, "cannot open script");
        scanner->scripts_failed++;
        return -1;
    }

    if (!read_u32_le_at(stream, 52, &num_entries_raw) || !read_u32_le_at(stream, 56, &max_entries_raw)) {
        scanner_issue(scanner, path, "cannot read script entry counts");
        fclose(stream);
        scanner->scripts_failed++;
        return 0;
    }

    if (num_entries_raw > max_entries_raw) {
        scanner_issue(scanner, path, "script num_entries %" PRIu32 " exceeds max_entries %" PRIu32, num_entries_raw, max_entries_raw);
        failed = true;
    }

    expected_size = SCRIPT_ENTRY_OFFSET + (uint64_t)num_entries_raw * SCRIPT_CONDITION_SIZE;
    if (size < expected_size) {
        scanner_issue(scanner, path, "script is truncated: expected at least %" PRIu64 " bytes, got %" PRIu64, expected_size, size);
        failed = true;
    }

    available_entries = size >= SCRIPT_ENTRY_OFFSET
        ? (size - SCRIPT_ENTRY_OFFSET) / SCRIPT_CONDITION_SIZE
        : 0;
    entries_to_read = num_entries_raw < available_entries ? num_entries_raw : available_entries;

    if (fseek(stream, SCRIPT_ENTRY_OFFSET, SEEK_SET) != 0) {
        scanner_issue(scanner, path, "cannot seek to script entries");
        fclose(stream);
        scanner->scripts_failed++;
        return 0;
    }

    for (index = 0; index < entries_to_read; index++) {
        int condition_type;
        int action_type;
        int else_type;

        if (fread(entry, sizeof(entry), 1, stream) != 1) {
            scanner_issue(scanner, path, "cannot read script entry %" PRIu64, index);
            failed = true;
            break;
        }

        condition_type = read_i32_le(entry);
        action_type = read_i32_le(entry + SCRIPT_CONDITION_ACTION_OFFSET);
        else_type = read_i32_le(entry + SCRIPT_CONDITION_ELSE_OFFSET);

        if (condition_type >= 0 && condition_type < 256) {
            scanner->script_condition_counts[condition_type]++;
        }
        if (action_type >= 0 && action_type < 256) {
            scanner->script_action_counts[action_type]++;
        }
        if (else_type >= 0 && else_type < 256) {
            scanner->script_action_counts[else_type]++;
        }

        if (condition_type < 0 || condition_type >= SCRIPT_CONDITION_TYPE_COUNT) {
            scanner_issue(scanner, path, "entry %" PRIu64 " uses unknown condition id %d", index, condition_type);
            scanner->script_condition_out_of_range++;
            failed = true;
        }
        if (action_type < 0 || action_type >= SCRIPT_ACTION_TYPE_COUNT) {
            scanner_issue(scanner, path, "entry %" PRIu64 " uses unknown action id %d", index, action_type);
            scanner->script_action_out_of_range++;
            failed = true;
        }
        if (else_type < 0 || else_type >= SCRIPT_ACTION_TYPE_COUNT) {
            scanner_issue(scanner, path, "entry %" PRIu64 " uses unknown else-action id %d", index, else_type);
            scanner->script_action_out_of_range++;
            failed = true;
        }
    }

    fclose(stream);

    scanner->script_entries += entries_to_read;
    if (failed) {
        scanner->scripts_failed++;
    } else {
        scanner->scripts_parsed++;
    }

    return 0;
}

int scan_dat_archive(Scanner* scanner, const char* path)
{
    FILE* stream;
    int32_t id;
    int32_t name_table_size;
    int32_t entry_table_offset;
    int32_t entry_table_size;
    int32_t entries_count;
    long file_size;
    unsigned int index;
    bool failed = false;

    stream = fopen(path, "rb");
    if (stream == NULL) {
        scanner_issue(scanner, path, "cannot open DAT archive");
        scanner->dats_failed++;
        return -1;
    }

    if (fseek(stream, 0, SEEK_END) != 0 || (file_size = ftell(stream)) < 12) {
        fclose(stream);
        return 0;
    }

    if (fseek(stream, -12, SEEK_END) != 0
        || !read_i32(stream, &id)
        || !read_i32(stream, &name_table_size)
        || !read_i32(stream, &entry_table_offset)) {
        scanner_issue(scanner, path, "cannot read DAT footer");
        fclose(stream);
        scanner->dats_failed++;
        return 0;
    }

    if (id != FOURCC_DAT && id != FOURCC_DAT1) {
        fclose(stream);
        return 0;
    }

    if (entry_table_offset <= 0 || entry_table_offset > file_size - 4) {
        scanner_issue(scanner, path, "cannot locate DAT entry table");
        fclose(stream);
        scanner->dats_failed++;
        return 0;
    }

    if (fseek(stream, -4 - entry_table_offset, SEEK_END) != 0
        || !read_i32(stream, &entry_table_size)
        || !read_i32(stream, &entries_count)
        || entry_table_size <= 0
        || entries_count < 0
        || name_table_size < 0) {
        scanner_issue(scanner, path, "cannot read DAT entry table header");
        fclose(stream);
        scanner->dats_failed++;
        return 0;
    }

    for (index = 0; index < (unsigned int)entries_count; index++) {
        uint32_t name_size;
        char* name;

        if (!read_u32(stream, &name_size) || name_size == 0 || name_size >= 4096) {
            scanner_issue(scanner, path, "invalid DAT entry name size at index %u", index);
            failed = true;
            break;
        }

        name = (char*)malloc(name_size + 1);
        if (name == NULL) {
            scanner_issue(scanner, path, "out of memory while reading DAT entry");
            fclose(stream);
            scanner->dats_failed++;
            return -1;
        }

        if (fread(name, name_size, 1, stream) != 1) {
            free(name);
            scanner_issue(scanner, path, "cannot read DAT entry name at index %u", index);
            failed = true;
            break;
        }
        name[name_size] = '\0';

        if (fseek(stream, 20, SEEK_CUR) != 0) {
            free(name);
            scanner_issue(scanner, path, "cannot skip DAT entry metadata at index %u", index);
            failed = true;
            break;
        }

        char* normalized = normalized_path_copy(name);
        free(name);
        if (normalized == NULL || !dat_entry_list_append(&scanner->dat_index_entries, path, normalized)) {
            free(normalized);
            scanner_issue(scanner, path, "out of memory while recording DAT entry");
            fclose(stream);
            scanner->dats_failed++;
            return -1;
        }
        scanner->dat_entries++;
    }

    fclose(stream);

    if (failed) {
        scanner->dats_failed++;
    } else {
        scanner->dats_indexed++;
    }

    return 0;
}

int scan_dialog(Scanner* scanner, const char* path)
{
    FILE* stream;
    DialogRecordList records;
    char num_field[DIALOG_FIELD_SIZE];
    char text_field[DIALOG_FIELD_SIZE];
    char gender_field[DIALOG_FIELD_SIZE];
    char iq_field[DIALOG_FIELD_SIZE];
    char conditions_field[DIALOG_FIELD_SIZE];
    char response_field[DIALOG_FIELD_SIZE];
    char actions_field[DIALOG_FIELD_SIZE];
    int line = 1;
    bool failed = false;

    memset(&records, 0, sizeof(records));

    stream = fopen(path, "rt");
    if (stream == NULL) {
        scanner_issue(scanner, path, "cannot open dialog");
        scanner->dialogs_failed++;
        return -1;
    }

    while (true) {
        DialogRecord record;
        FieldResult result;
        int entry_line = line;

        result = read_braced_field(scanner, stream, path, num_field, sizeof(num_field), &line, true);
        if (result == FIELD_RESULT_EOF) {
            break;
        }
        if (result == FIELD_RESULT_ERROR) {
            failed = true;
            break;
        }

        if (read_braced_field(scanner, stream, path, text_field, sizeof(text_field), &line, true) != FIELD_RESULT_OK
            || read_braced_field(scanner, stream, path, gender_field, sizeof(gender_field), &line, true) != FIELD_RESULT_OK
            || read_braced_field(scanner, stream, path, iq_field, sizeof(iq_field), &line, true) != FIELD_RESULT_OK
            || read_braced_field(scanner, stream, path, conditions_field, sizeof(conditions_field), &line, true) != FIELD_RESULT_OK
            || read_braced_field(scanner, stream, path, response_field, sizeof(response_field), &line, true) != FIELD_RESULT_OK
            || read_braced_field(scanner, stream, path, actions_field, sizeof(actions_field), &line, true) != FIELD_RESULT_OK) {
            scanner_issue(scanner, path, "incomplete dialog entry beginning on line %d", entry_line);
            failed = true;
            break;
        }

        if (!parse_int_field(num_field, &record.num)) {
            scanner_issue(scanner, path, "dialog entry on line %d has invalid line number '%s'", entry_line, num_field);
            failed = true;
            record.num = 0;
        }

        if (!parse_int_field(iq_field, &record.iq)) {
            if (iq_field[0] != '\0') {
                scanner_issue(scanner, path, "dialog line %d has invalid minimum IQ '%s'", record.num, iq_field);
                failed = true;
            }
            record.iq = 0;
        }

        if (!parse_int_field(response_field, &record.response_val)) {
            scanner_issue(scanner, path, "dialog line %d has invalid response value '%s'", record.num, response_field);
            failed = true;
            record.response_val = 0;
        }

        if (response_field[0] == '#') {
            scanner_issue(scanner, path, "dialog line %d has comment marker in response value", record.num);
            failed = true;
        }

        record.line = entry_line;

        if (dialog_record_list_find(&records, record.num) != -1) {
            scanner_issue(scanner, path, "duplicate dialog line %d", record.num);
            failed = true;
        }

        if (!dialog_record_list_append(&records, record)) {
            scanner_issue(scanner, path, "out of memory while recording dialog line");
            failed = true;
            break;
        }
    }

    fclose(stream);

    for (size_t index = 0; index < records.length; index++) {
        DialogRecord* record = &(records.values[index]);

        if (record->iq != 0 && record->response_val > 0) {
            int target = dialog_record_list_find(&records, record->response_val);

            if (target == -1) {
                scanner_issue(scanner, path, "PC response %d jumps to non-existent dialog line %d", record->num, record->response_val);
                failed = true;
            } else if (records.values[target].iq != 0) {
                scanner_issue(scanner, path, "PC response %d jumps to non-PC line %d", record->num, record->response_val);
                failed = true;
            }
        }
    }

    scanner->dialog_entries += records.length;
    if (failed) {
        scanner->dialogs_failed++;
    } else {
        scanner->dialogs_parsed++;
    }

    dialog_record_list_free(&records);

    return 0;
}

int scan_message(Scanner* scanner, const char* path)
{
    FILE* stream;
    IntList ids;
    char num_field[MES_FIELD_SIZE];
    char text_field[MES_FIELD_SIZE];
    int line = 1;
    bool failed = false;

    memset(&ids, 0, sizeof(ids));

    stream = fopen(path, "rt");
    if (stream == NULL) {
        scanner_issue(scanner, path, "cannot open message file");
        scanner->messages_failed++;
        return -1;
    }

    while (true) {
        FieldResult result;
        int num;
        int entry_line = line;

        result = read_braced_field(scanner, stream, path, num_field, sizeof(num_field), &line, false);
        if (result == FIELD_RESULT_EOF) {
            break;
        }
        if (result == FIELD_RESULT_ERROR) {
            failed = true;
            break;
        }

        if (read_braced_field(scanner, stream, path, text_field, sizeof(text_field), &line, false) != FIELD_RESULT_OK) {
            scanner_issue(scanner, path, "missing message text for entry beginning on line %d", entry_line);
            failed = true;
            break;
        }

        if (!parse_int_field(num_field, &num)) {
            scanner_issue(scanner, path, "message entry on line %d has invalid number '%s'", entry_line, num_field);
            failed = true;
            num = 0;
        }

        if (int_list_contains(&ids, num)) {
            scanner_issue(scanner, path, "duplicate message line %d", num);
            failed = true;
        } else if (!int_list_append(&ids, num)) {
            scanner_issue(scanner, path, "out of memory while recording message line");
            failed = true;
            break;
        }
    }

    fclose(stream);

    scanner->message_entries += ids.length;
    if (failed) {
        scanner->messages_failed++;
    } else {
        scanner->messages_parsed++;
    }

    int_list_free(&ids);

    return 0;
}

FieldResult read_braced_field(Scanner* scanner, FILE* stream, const char* path, char* buffer, size_t size, int* line_ptr, bool skip_line_comments)
{
    int ch;
    int prev = 0;
    size_t len = 0;
    bool overflow = false;

    ch = fgetc(stream);
    while (ch != '{') {
        if (ch == EOF) {
            return FIELD_RESULT_EOF;
        }

        if (ch == '\n') {
            (*line_ptr)++;
        } else if (ch == '}') {
            scanner_issue(scanner, path, "possible missing left brace on or before line %d", *line_ptr);
        } else if (skip_line_comments && ch == '/' && prev == '/') {
            do {
                ch = fgetc(stream);
                if (ch == EOF) {
                    return FIELD_RESULT_EOF;
                }
            } while (ch != '\n');

            (*line_ptr)++;
            prev = 0;
            ch = fgetc(stream);
            continue;
        } else {
            prev = ch;
        }

        ch = fgetc(stream);
    }

    ch = fgetc(stream);
    while (ch != '}') {
        if (ch == EOF) {
            scanner_issue(scanner, path, "expected right brace before end of file");
            return FIELD_RESULT_ERROR;
        }

        if (ch == '\n') {
            (*line_ptr)++;
        } else if (ch == '{') {
            scanner_issue(scanner, path, "possible missing right brace on or before line %d", *line_ptr);
        }

        if (len + 1 < size) {
            buffer[len++] = (char)(unsigned char)ch;
        } else {
            overflow = true;
        }

        ch = fgetc(stream);
    }

    buffer[len] = '\0';

    if (overflow) {
        scanner_issue(scanner, path, "field too long on or before line %d", *line_ptr);
    }

    return FIELD_RESULT_OK;
}

FileKind file_kind_from_path(const char* path)
{
    if (has_extension(path, ".scr")) return FILE_KIND_SCRIPT;
    if (has_extension(path, ".dlg")) return FILE_KIND_DIALOG;
    if (has_extension(path, ".mes")) return FILE_KIND_MESSAGE;
    if (has_extension(path, ".pro")) return FILE_KIND_PROTOTYPE;
    if (has_extension(path, ".mob")) return FILE_KIND_MOBILE;
    if (has_extension(path, ".sec")) return FILE_KIND_SECTOR;
    if (has_extension(path, ".map")) return FILE_KIND_MAP;
    if (has_extension(path, ".art")) return FILE_KIND_ART;
    if (has_extension(path, ".dat")) return FILE_KIND_DAT;
    if (has_extension(path, ".wav") || has_extension(path, ".mp3") || has_extension(path, ".ogg")) return FILE_KIND_SOUND;
    return FILE_KIND_OTHER;
}

const char* file_kind_name(FileKind kind)
{
    switch (kind) {
    case FILE_KIND_SCRIPT:
        return "scr";
    case FILE_KIND_DIALOG:
        return "dlg";
    case FILE_KIND_MESSAGE:
        return "mes";
    case FILE_KIND_PROTOTYPE:
        return "pro";
    case FILE_KIND_MOBILE:
        return "mob";
    case FILE_KIND_SECTOR:
        return "sec";
    case FILE_KIND_MAP:
        return "map";
    case FILE_KIND_ART:
        return "art";
    case FILE_KIND_SOUND:
        return "sound";
    case FILE_KIND_DAT:
        return "dat";
    case FILE_KIND_OTHER:
        return "other";
    default:
        return "unknown";
    }
}

const char* file_name(const char* path)
{
    const char* slash = strrchr(path, '/');
    const char* backslash = strrchr(path, '\\');
    const char* sep;

    if (slash == NULL) {
        sep = backslash;
    } else if (backslash == NULL) {
        sep = slash;
    } else {
        sep = slash > backslash ? slash : backslash;
    }

    return sep != NULL ? sep + 1 : path;
}

const char* relative_path(const Scanner* scanner, const char* path)
{
    const char* rel;

    if (scanner->root_path == NULL) {
        return path;
    }

    if (strncmp(path, scanner->root_path, scanner->root_path_len) != 0) {
        return path;
    }

    rel = path + scanner->root_path_len;
    while (*rel == '/' || *rel == '\\') {
        rel++;
    }

    return rel;
}

char* join_path(const char* base, const char* name)
{
    size_t base_len = strlen(base);
    size_t name_len = strlen(name);
    bool needs_sep = base_len > 0 && base[base_len - 1] != '/' && base[base_len - 1] != '\\';
    char* path = (char*)malloc(base_len + name_len + (needs_sep ? 2 : 1));

    if (path == NULL) {
        return NULL;
    }

    memcpy(path, base, base_len);
    if (needs_sep) {
#ifdef _WIN32
        path[base_len++] = '\\';
#else
        path[base_len++] = '/';
#endif
    }
    memcpy(path + base_len, name, name_len + 1);

    return path;
}

char* string_duplicate(const char* str)
{
    size_t len = strlen(str);
    char* copy = (char*)malloc(len + 1);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, str, len + 1);
    return copy;
}

char* normalized_path_copy(const char* path)
{
    char* normalized = string_duplicate(path);

    if (normalized == NULL) {
        return NULL;
    }

    for (char* ch = normalized; *ch != '\0'; ch++) {
        if (*ch == '/') {
            *ch = '\\';
        } else {
            *ch = (char)tolower((unsigned char)*ch);
        }
    }

    return normalized;
}

bool has_extension(const char* path, const char* extension)
{
    size_t path_len = strlen(path);
    size_t ext_len = strlen(extension);

    if (path_len < ext_len) {
        return false;
    }

    path += path_len - ext_len;
    while (*path != '\0' && *extension != '\0') {
        if (tolower((unsigned char)*path) != tolower((unsigned char)*extension)) {
            return false;
        }

        path++;
        extension++;
    }

    return true;
}

bool parse_int_field(const char* str, int* value_ptr)
{
    char* end;
    long value;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return false;
    }

    value = strtol(str, &end, 10);
    if (end == str) {
        return false;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0') {
        return false;
    }

    *value_ptr = (int)value;
    return true;
}

bool parse_leading_int(const char* str, int* value_ptr)
{
    int value = 0;
    bool found = false;

    while (isdigit((unsigned char)*str)) {
        found = true;
        value = value * 10 + (*str - '0');
        str++;
    }

    if (!found) {
        return false;
    }

    *value_ptr = value;
    return true;
}

int dialog_record_list_find(const DialogRecordList* list, int num)
{
    for (size_t index = 0; index < list->length; index++) {
        if (list->values[index].num == num) {
            return (int)index;
        }
    }

    return -1;
}

bool dialog_record_list_append(DialogRecordList* list, DialogRecord record)
{
    DialogRecord* values;
    size_t capacity;

    if (list->length == list->capacity) {
        capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        values = (DialogRecord*)realloc(list->values, sizeof(*values) * capacity);
        if (values == NULL) {
            return false;
        }

        list->values = values;
        list->capacity = capacity;
    }

    list->values[list->length++] = record;
    return true;
}

void dialog_record_list_free(DialogRecordList* list)
{
    free(list->values);
    list->values = NULL;
    list->length = 0;
    list->capacity = 0;
}

bool int_list_contains(const IntList* list, int value)
{
    for (size_t index = 0; index < list->length; index++) {
        if (list->values[index] == value) {
            return true;
        }
    }

    return false;
}

bool int_list_append(IntList* list, int value)
{
    int* values;
    size_t capacity;

    if (list->length == list->capacity) {
        capacity = list->capacity == 0 ? 32 : list->capacity * 2;
        values = (int*)realloc(list->values, sizeof(*values) * capacity);
        if (values == NULL) {
            return false;
        }

        list->values = values;
        list->capacity = capacity;
    }

    list->values[list->length++] = value;
    return true;
}

void int_list_free(IntList* list)
{
    free(list->values);
    list->values = NULL;
    list->length = 0;
    list->capacity = 0;
}

bool string_list_contains(const StringList* list, const char* value)
{
    for (size_t index = 0; index < list->length; index++) {
        if (strcmp(list->values[index], value) == 0) {
            return true;
        }
    }

    return false;
}

bool string_list_append(StringList* list, char* value)
{
    char** values;
    size_t capacity;

    if (list->length == list->capacity) {
        capacity = list->capacity == 0 ? 64 : list->capacity * 2;
        values = (char**)realloc(list->values, sizeof(*values) * capacity);
        if (values == NULL) {
            return false;
        }

        list->values = values;
        list->capacity = capacity;
    }

    list->values[list->length++] = value;
    return true;
}

void string_list_free(StringList* list)
{
    for (size_t index = 0; index < list->length; index++) {
        free(list->values[index]);
    }

    free(list->values);
    list->values = NULL;
    list->length = 0;
    list->capacity = 0;
}

bool dat_entry_list_append(DatEntryList* list, const char* dat_path, char* entry_path)
{
    DatEntry* values;
    char* dat_path_copy;
    size_t capacity;

    if (list->length == list->capacity) {
        capacity = list->capacity == 0 ? 128 : list->capacity * 2;
        values = (DatEntry*)realloc(list->values, sizeof(*values) * capacity);
        if (values == NULL) {
            return false;
        }

        list->values = values;
        list->capacity = capacity;
    }

    dat_path_copy = string_duplicate(dat_path);
    if (dat_path_copy == NULL) {
        return false;
    }

    list->values[list->length].dat_path = dat_path_copy;
    list->values[list->length].entry_path = entry_path;
    list->length++;
    return true;
}

void dat_entry_list_free(DatEntryList* list)
{
    for (size_t index = 0; index < list->length; index++) {
        free(list->values[index].dat_path);
        free(list->values[index].entry_path);
    }

    free(list->values);
    list->values = NULL;
    list->length = 0;
    list->capacity = 0;
}

int read_i32_le(const unsigned char* bytes)
{
    uint32_t value = (uint32_t)bytes[0]
        | ((uint32_t)bytes[1] << 8)
        | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 24);

    return (int32_t)value;
}

bool read_i32(FILE* stream, int32_t* value_ptr)
{
    unsigned char bytes[4];

    if (fread(bytes, sizeof(bytes), 1, stream) != 1) {
        return false;
    }

    *value_ptr = read_i32_le(bytes);
    return true;
}

bool read_u32(FILE* stream, uint32_t* value_ptr)
{
    int32_t value;

    if (!read_i32(stream, &value)) {
        return false;
    }

    *value_ptr = (uint32_t)value;
    return true;
}

bool read_u32_le_at(FILE* stream, long offset, uint32_t* value_ptr)
{
    unsigned char bytes[4];

    if (fseek(stream, offset, SEEK_SET) != 0) {
        return false;
    }

    if (fread(bytes, sizeof(bytes), 1, stream) != 1) {
        return false;
    }

    *value_ptr = (uint32_t)bytes[0]
        | ((uint32_t)bytes[1] << 8)
        | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 24);
    return true;
}

void print_dat_override_report(const Scanner* scanner)
{
    uint64_t overrides = 0;

    printf("\nDAT/loose override report:\n");

    for (size_t index = 0; index < scanner->dat_index_entries.length; index++) {
        const DatEntry* entry = &(scanner->dat_index_entries.values[index]);
        if (string_list_contains(&scanner->loose_paths, entry->entry_path)) {
            overrides++;
        }
    }

    printf("  loose files tracked: %zu\n", scanner->loose_paths.length);
    printf("  loose files overriding DAT entries: %" PRIu64 "\n", overrides);

    if (overrides == 0) {
        return;
    }

    for (size_t index = 0; index < scanner->dat_index_entries.length; index++) {
        const DatEntry* entry = &(scanner->dat_index_entries.values[index]);
        if (string_list_contains(&scanner->loose_paths, entry->entry_path)) {
            printf("  override: %s (archive: %s)\n", entry->entry_path, entry->dat_path);
        }
    }
}

bool profile_check_load(Scanner* scanner, const char* path, ProfileCheck* profile)
{
    FILE* stream;
    char line[512];

    snprintf(profile->path, sizeof(profile->path), "%s", path);

    stream = fopen(path, "rt");
    if (stream == NULL) {
        scanner_issue(scanner, path, "cannot open profile");
        return false;
    }

    while (fgets(line, sizeof(line), stream) != NULL) {
        profile_check_parse_line(profile, line);
    }

    fclose(stream);
    profile->loaded = true;
    return true;
}

void profile_check_parse_line(ProfileCheck* profile, char* line)
{
    char* sep;
    char* key_start;
    char* key_end;
    char* value_start;
    char* value_end;

    profile_check_strip_comments(line);

    key_start = line;
    key_end = key_start + strlen(key_start);
    profile_check_trim(&key_start, &key_end);
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
    profile_check_trim(&key_start, &key_end);
    *key_end = '\0';

    value_start = sep + 1;
    value_end = value_start + strlen(value_start);
    profile_check_trim(&value_start, &value_end);
    *value_end = '\0';

    if (profile_check_key_equals(key_start, "Module")) {
        snprintf(profile->module, sizeof(profile->module), "%s", value_start);
    } else if (profile_check_key_equals(key_start, "ExpectedPath")
        || profile_check_key_equals(key_start, "ExpectPath")
        || profile_check_key_equals(key_start, "ExpectedDat")
        || profile_check_key_equals(key_start, "ExpectDat")
        || profile_check_key_equals(key_start, "ExpectedRoot")
        || profile_check_key_equals(key_start, "ExpectRoot")
        || profile_check_key_equals(key_start, "ExpectedModuleRoot")
        || profile_check_key_equals(key_start, "ExpectedLooseRoot")) {
        profile_check_add_expected_path(profile, value_start);
    }
}

void profile_check_add_expected_path(ProfileCheck* profile, const char* path)
{
    if (path[0] == '\0'
        || profile->expected_path_count >= PROFILE_CHECK_MAX_EXPECTED_PATHS) {
        return;
    }

    snprintf(profile->expected_paths[profile->expected_path_count],
        sizeof(profile->expected_paths[profile->expected_path_count]),
        "%s",
        path);
    profile->expected_path_count++;
}

void profile_check_apply(Scanner* scanner, const char* root_path, const ProfileCheck* profile)
{
    char path[PROFILE_CHECK_MAX_PATH * 2];

    if (!profile->enabled || !profile->loaded) {
        return;
    }

    if (profile->module[0] != '\0') {
        if (profile_check_is_absolute_or_relative(profile->module)) {
            snprintf(path, sizeof(path), "%s", profile->module);
        } else {
            snprintf(path, sizeof(path), "%s\\Modules\\%s", root_path, profile->module);
        }

        char module_dat[PROFILE_CHECK_MAX_PATH * 2];
        snprintf(module_dat, sizeof(module_dat), "%s.dat", path);
        if (!profile_check_path_exists(path) && !profile_check_path_exists(module_dat)) {
            scanner_issue(scanner, profile->path, "profile module '%s' is missing '%s' or '%s'", profile->module, path, module_dat);
        }
    }

    for (size_t index = 0; index < profile->expected_path_count; index++) {
        const char* expected_path = profile->expected_paths[index];

        if (profile_check_is_absolute_or_relative(expected_path)) {
            snprintf(path, sizeof(path), "%s", expected_path);
        } else {
            snprintf(path, sizeof(path), "%s\\%s", root_path, expected_path);
        }

        if (!profile_check_path_exists(path)) {
            scanner_issue(scanner, profile->path, "expected profile path is missing: %s", path);
        }
    }
}

void profile_check_print(const ProfileCheck* profile)
{
    printf("\nProfile check:\n");

    if (!profile->enabled) {
        printf("  enabled: 0\n");
        return;
    }

    printf("  enabled: 1\n");
    printf("  path: %s\n", profile->path);
    printf("  loaded: %d\n", profile->loaded ? 1 : 0);
    printf("  module: %s\n", profile->module[0] != '\0' ? profile->module : "(none)");
    printf("  expected paths: %zu\n", profile->expected_path_count);
    for (size_t index = 0; index < profile->expected_path_count; index++) {
        printf("    %s\n", profile->expected_paths[index]);
    }
}

void profile_check_strip_comments(char* line)
{
    char* comment = strstr(line, "//");
    char* hash = strchr(line, '#');
    char* semicolon = strchr(line, ';');

    if (hash != NULL && (comment == NULL || hash < comment)) {
        comment = hash;
    }

    if (semicolon != NULL && (comment == NULL || semicolon < comment)) {
        comment = semicolon;
    }

    if (comment != NULL) {
        *comment = '\0';
    }
}

void profile_check_trim(char** start_ptr, char** end_ptr)
{
    while (*start_ptr < *end_ptr && isspace((unsigned char)**start_ptr)) {
        (*start_ptr)++;
    }

    while (*end_ptr > *start_ptr && isspace((unsigned char)*((*end_ptr) - 1))) {
        (*end_ptr)--;
    }
}

bool profile_check_key_equals(const char* lhs, const char* rhs)
{
    while (*lhs != '\0' || *rhs != '\0') {
        while (*lhs == '_' || *lhs == '-') {
            lhs++;
        }

        while (*rhs == '_' || *rhs == '-') {
            rhs++;
        }

        if (tolower((unsigned char)*lhs) != tolower((unsigned char)*rhs)) {
            return false;
        }

        if (*lhs == '\0') {
            return true;
        }

        lhs++;
        rhs++;
    }

    return true;
}

bool profile_check_path_exists(const char* path)
{
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

bool profile_check_is_absolute_or_relative(const char* path)
{
    return path[0] == '\\'
        || path[0] == '/'
        || path[0] == '.'
        || (isalpha((unsigned char)path[0]) && path[1] == ':');
}

void scanner_issue(Scanner* scanner, const char* path, const char* format, ...)
{
    va_list args;

    scanner->issues++;
    printf("warning: %s: ", path);

    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

void print_histogram(const char* title, const uint64_t* values, size_t count, size_t known_count)
{
    bool any = false;

    printf("%s:", title);
    for (size_t index = 0; index < count; index++) {
        if (values[index] != 0) {
            if (!any) {
                printf("\n");
                any = true;
            }

            printf("    %zu%s: %" PRIu64 "\n",
                index,
                index < known_count ? "" : " (unknown)",
                values[index]);
        }
    }

    if (!any) {
        printf(" none\n");
    }
}
