include(FetchContent)

set(ZLIB_BUILD_TESTING OFF)
set(ZLIB_BUILD_SHARED OFF)

FetchContent_Declare(zlib
    GIT_REPOSITORY "https://github.com/madler/zlib"
    GIT_TAG "v1.3.2"
)

FetchContent_MakeAvailable(zlib)

set(ZLIB_LIBRARIES "zlibstatic" PARENT_SCOPE)
set(ZLIB_INCLUDE_DIRS "${zlib_SOURCE_DIR}" "${zlib_BINARY_DIR}" PARENT_SCOPE)
