#include "tig/debug.h"

#include <time.h>

#include "tig/memory.h"

// 0x4FEB10
int tig_debug_init(TigInitInfo* init_info)
{
    time_t now;

    (void)init_info;

    tig_memory_set_output_func(tig_debug_println);
    tig_debug_println("\n");

    time(&now);
    tig_debug_println(asctime(localtime(&now)));

    return 0;
}

// 0x4FEB60
void tig_debug_exit()
{
}

// 0x4FEB70
void tig_debug_printf(const char* format, ...)
{
    // 0x604214
    static char buffer[1024];

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
    len = SDL_strlcat(buffer, tmp, sizeof(buffer));
    if (len != 0 && (len == sizeof(buffer) - 1 || buffer[len - 1] == '\n')) {
        SDL_Log("%s", buffer);
        buffer[0] = '\0';
    }

    va_end(args);
}

// 0x4FEBC0
void tig_debug_println(const char* string)
{
    tig_debug_printf("%s\n", string);
}
