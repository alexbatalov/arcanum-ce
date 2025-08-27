set(SDL_SHARED OFF)
set(SDL_STATIC ON)

include(FetchContent)

FetchContent_Declare(SDL3
    GIT_REPOSITORY "https://github.com/libsdl-org/SDL"
    GIT_TAG "main"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL3)
