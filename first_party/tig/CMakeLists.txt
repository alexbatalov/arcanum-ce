# Keep alphabetically sorted.
set(HDRS
    "include/tig/art.h"
    "include/tig/bmp.h"
    "include/tig/bsearch.h"
    "include/tig/button.h"
    "include/tig/color.h"
    "include/tig/compat.h"
    "include/tig/core.h"
    "include/tig/database.h"
    "include/tig/debug.h"
    "include/tig/draw.h"
    "include/tig/file_cache.h"
    "include/tig/file.h"
    "include/tig/find_file.h"
    "include/tig/font.h"
    "include/tig/guid.h"
    "include/tig/idxtable.h"
    "include/tig/kb.h"
    "include/tig/memory.h"
    "include/tig/message.h"
    "include/tig/mouse.h"
    "include/tig/movie.h"
    "include/tig/palette.h"
    "include/tig/rect.h"
    "include/tig/sound.h"
    "include/tig/str_parse.h"
    "include/tig/tig.h"
    "include/tig/timer.h"
    "include/tig/video.h"
    "include/tig/window.h"
)

# Keep alphabetically sorted.
set(SRCS
    "src/art.c"
    "src/bmp.c"
    "src/bsearch.c"
    "src/button.c"
    "src/color.c"
    "src/compat.c"
    "src/core.c"
    "src/database.c"
    "src/debug.c"
    "src/draw.c"
    "src/file_cache.c"
    "src/file.c"
    "src/find_file.c"
    "src/font.c"
    "src/guid.c"
    "src/idxtable.c"
    "src/kb.c"
    "src/memory.c"
    "src/message.c"
    "src/mouse.c"
    "src/movie.c"
    "src/palette.c"
    "src/rect.c"
    "src/sound.c"
    "src/str_parse.c"
    "src/timer.c"
    "src/video.c"
    "src/window.c"
)

add_library(tig_common INTERFACE)

target_include_directories(tig_common INTERFACE
    ${ZLIB_INCLUDE_DIRS}
    ${BINK_COMPAT_INCLUDE_DIR}
    ${MSS_COMPAT_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(tig_common INTERFACE
    fpattern::fpattern
    SDL3::SDL3
    SDL3_mixer::SDL3_mixer
    ${ZLIB_LIBRARIES}
    ${BINK_COMPAT_LIBRARY}
    ${MSS_COMPAT_LIBRARY}
)

if(WIN32)
    target_compile_definitions(tig_common INTERFACE
        _CRT_NONSTDC_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS
    )
endif()

add_library(tig STATIC
    ${HDRS}
    ${SRCS}
)

# Derive settings from common library.
target_link_libraries(tig PUBLIC tig_common)

# Turn on strict mode (but only for the library itself, hence PRIVATE).
if(MSVC)
    target_compile_options(tig PRIVATE /W4)
else()
    target_compile_options(tig PRIVATE -Wall -Wextra)
endif()

set(TIG_LIBRARY "tig" PARENT_SCOPE)
set(TIG_INCLUDE_DIR "${tig_SOURCE_DIR}/include" PARENT_SCOPE)
