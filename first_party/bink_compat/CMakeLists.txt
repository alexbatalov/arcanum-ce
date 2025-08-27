add_library(bink_compat STATIC
    "include/bink_compat.h"
    "src/bink_compat.c"
)

target_include_directories(bink_compat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(BINK_COMPAT_LIBRARY "bink_compat" PARENT_SCOPE)
set(BINK_COMPAT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
