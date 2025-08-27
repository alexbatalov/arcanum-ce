add_library(mss_compat STATIC
    "include/mss_compat.h"
    "src/mss_compat.c"
)

target_include_directories(mss_compat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(mss_compat PRIVATE SDL3_mixer::SDL3_mixer)

set(MSS_COMPAT_LIBRARY "mss_compat" PARENT_SCOPE)
set(MSS_COMPAT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
