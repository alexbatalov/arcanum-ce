option(ARCANUM_BINK_FFMPEG "Use the FFmpeg backend for Bink video playback (LGPL, dynamically linked)" OFF)

if(ARCANUM_BINK_FFMPEG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED
        libavcodec
        libavformat
        libavutil
        libswscale
        libswresample
    )

    add_library(bink_compat STATIC
        "include/bink_compat.h"
        "src/bink_compat_ffmpeg.c"
    )

    # Use FFmpeg headers for types and version macros, but do NOT link the
    # libraries. They are loaded lazily at runtime via dlopen so the game still
    # launches (and simply skips movies) when FFmpeg is not installed.
    #
    # Pass the pkg-config library directory so dlopen can find the libraries
    # even when they live outside the loader's default search path (e.g.
    # Homebrew on macOS). The loader's default path is tried as a fallback.
    pkg_get_variable(FFMPEG_LIBDIR libavcodec libdir)
    target_include_directories(bink_compat PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_compile_definitions(bink_compat PRIVATE "FFMPEG_LIBDIR=\"${FFMPEG_LIBDIR}\"")
    target_link_libraries(bink_compat PRIVATE SDL3::SDL3 ${CMAKE_DL_LIBS})
else()
    add_library(bink_compat STATIC
        "include/bink_compat.h"
        "src/bink_compat.c"
    )
endif()

target_include_directories(bink_compat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(BINK_COMPAT_LIBRARY "bink_compat" PARENT_SCOPE)
set(BINK_COMPAT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
