#include "tig/debug.h"

#include <time.h>

#include "tig/memory.h"

#define TIG_DEBUG_LINE_BUFFER_SIZE 1024
#define TIG_DEBUG_EARLY_BUFFER_SIZE 8192

static void tig_debug_output(const char* string);
static void tig_debug_append_early(const char* string);

// 0x604214
static char tig_debug_line_buffer[TIG_DEBUG_LINE_BUFFER_SIZE];

static bool tig_debug_initialized;
static char tig_debug_early_buffer[TIG_DEBUG_EARLY_BUFFER_SIZE];
static size_t tig_debug_early_buffer_len;
static size_t tig_debug_early_buffer_dropped;

// 0x4FEB10
int tig_debug_init(TigInitInfo* init_info)
{
    time_t now;

    (void)init_info;

    tig_debug_initialized = true;
    tig_debug_flush();

    tig_memory_set_output_func(tig_debug_println);
    tig_debug_println("\n");

    time(&now);
    tig_debug_println(asctime(localtime(&now)));

    return 0;
}

// 0x4FEB60
void tig_debug_exit(void)
{
    tig_debug_flush();
}

// 0x4FEB70
void tig_debug_printf(const char* format, ...)
{
    va_list args;
    size_t len;
    char tmp[1024];

    va_start(args, format);
    SDL_vsnprintf(tmp, sizeof(tmp), format, args);

    // NOTE: Typically, debug messages in Arcanum conclude with a newline.
    // However, in certain situations where the developers intended to display
    // progress-like messages on a single line, multiple consecutive messages
    // are utilized, such as "loading..." followed by "done, took: %d ms\n". SDL
    // does not permit such prints - it always appends a newline at the end of
    // log messages (for thread-safety reasons), which reduces the readability
    // of the logs. To support the previous approach, messages that do not end
    // with a newline are buffered.
    len = SDL_strlcat(tig_debug_line_buffer, tmp, sizeof(tig_debug_line_buffer));
    if (len != 0
        && (len == sizeof(tig_debug_line_buffer) - 1 || tig_debug_line_buffer[len - 1] == '\n')) {
        tig_debug_output(tig_debug_line_buffer);
        tig_debug_line_buffer[0] = '\0';
    }

    va_end(args);
}

// 0x4FEBC0
void tig_debug_println(const char* string)
{
    tig_debug_printf("%s\n", string);
}

void tig_debug_flush(void)
{
    if (tig_debug_early_buffer_len != 0) {
        SDL_Log("%s", tig_debug_early_buffer);
        tig_debug_early_buffer[0] = '\0';
        tig_debug_early_buffer_len = 0;
    }

    if (tig_debug_line_buffer[0] != '\0') {
        SDL_Log("%s", tig_debug_line_buffer);
        tig_debug_line_buffer[0] = '\0';
    }

    if (tig_debug_early_buffer_dropped != 0) {
        SDL_Log("TIG Debug: early log buffer truncated; %zu byte(s) dropped.",
            tig_debug_early_buffer_dropped);
        tig_debug_early_buffer_dropped = 0;
    }
}

static void tig_debug_output(const char* string)
{
    if (tig_debug_initialized) {
        SDL_Log("%s", string);
    } else {
        tig_debug_append_early(string);
    }
}

static void tig_debug_append_early(const char* string)
{
    size_t remaining;
    size_t len;

    remaining = sizeof(tig_debug_early_buffer) - tig_debug_early_buffer_len;
    len = SDL_strlen(string);

    if (remaining > 1) {
        size_t copied;

        copied = SDL_strlcpy(&(tig_debug_early_buffer[tig_debug_early_buffer_len]), string, remaining);
        if (copied >= remaining) {
            tig_debug_early_buffer_len = sizeof(tig_debug_early_buffer) - 1;
            tig_debug_early_buffer_dropped += copied - remaining + 1;
        } else {
            tig_debug_early_buffer_len += copied;
        }
    } else {
        tig_debug_early_buffer_dropped += len;
    }
}
