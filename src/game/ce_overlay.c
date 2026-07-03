#include "game/ce_overlay.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#include "tig/debug.h"
#include "tig/file.h"

#define CE_OVERLAY_ROOT "ce"

static bool ce_overlay_mounted;

static bool ce_overlay_root_exists(void);

bool ce_overlay_mount(void)
{
    if (ce_overlay_mounted) {
        return true;
    }

    if (!ce_overlay_root_exists()) {
        tig_debug_printf("Arcanum CE: ce/ overlay not found, continuing normally.\n");
        tig_debug_printf("Arcanum CE: ce/ overlay root path: %s\n", CE_OVERLAY_ROOT);
        return false;
    }

    if (!tig_file_repository_add(CE_OVERLAY_ROOT)) {
        tig_debug_printf("Arcanum CE: ce/ overlay found but could not be mounted: %s\n", CE_OVERLAY_ROOT);
        return false;
    }

    ce_overlay_mounted = true;

    tig_debug_printf("Arcanum CE: ce/ overlay mounted.\n");
    tig_debug_printf("Arcanum CE: ce/ overlay root path: %s\n", CE_OVERLAY_ROOT);

    return true;
}

bool ce_overlay_is_mounted(void)
{
    return ce_overlay_mounted;
}

const char* ce_overlay_root_path(void)
{
    return CE_OVERLAY_ROOT;
}

bool ce_overlay_root_exists(void)
{
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA(CE_OVERLAY_ROOT);

    return attributes != INVALID_FILE_ATTRIBUTES
        && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st;

    return stat(CE_OVERLAY_ROOT, &st) == 0
        && S_ISDIR(st.st_mode);
#endif
}
