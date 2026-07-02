#include "bink_compat.h"

#include <stddef.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

typedef void(BINKCALL* BINKCLOSE)(HBINK);
typedef int(BINKCALL* BINKCOPYTOBUFFER)(HBINK, void*, int, unsigned, unsigned, unsigned, unsigned);
typedef int(BINKCALL* BINKDDSURFACETYPE)(void*);
typedef int(BINKCALL* BINKDOFRAME)(HBINK);
typedef void(BINKCALL* BINKNEXTFRAME)(HBINK);
typedef HBINK(BINKCALL* BINKOPEN)(const char*, unsigned);
typedef BINKSNDOPEN(BINKCALL* BINKOPENMILES)(void*);
typedef int(BINKCALL* BINKSETSOUNDSYSTEM)(BINKSNDSYSOPEN, void*);
typedef void(BINKCALL* BINKSETSOUNDTRACK)(unsigned);
typedef int(BINKCALL* BINKWAIT)(HBINK);

#ifdef _WIN32
static HMODULE bink_library;

static FARPROC bink_compat_get_proc(const char* decorated_name, const char* name);
#endif

static BINKCLOSE _BinkClose;
static BINKCOPYTOBUFFER _BinkCopyToBuffer;
static BINKDDSURFACETYPE _BinkDDSurfaceType;
static BINKDOFRAME _BinkDoFrame;
static BINKNEXTFRAME _BinkNextFrame;
static BINKOPEN _BinkOpen;
static BINKOPENMILES _BinkOpenMiles;
static BINKSETSOUNDSYSTEM _BinkSetSoundSystem;
static BINKSETSOUNDTRACK _BinkSetSoundTrack;
static BINKWAIT _BinkWait;

void BINKCALL BinkClose(HBINK bnk)
{
    if (_BinkClose != NULL) {
        _BinkClose(bnk);
    }
}

int BINKCALL BinkCopyToBuffer(HBINK bnk, void* dest, int destpitch, unsigned destheight, unsigned destx, unsigned desty, unsigned flags)
{
    if (_BinkCopyToBuffer != NULL) {
        return _BinkCopyToBuffer(bnk, dest, destpitch, destheight, destx, desty, flags);
    } else {
        return -1;
    }
}

int BINKCALL BinkDDSurfaceType(void* lpDDS)
{
    if (_BinkDDSurfaceType != NULL) {
        return _BinkDDSurfaceType(lpDDS);
    } else {
        return -1;
    }
}

int BINKCALL BinkDoFrame(HBINK bnk)
{
    if (_BinkDoFrame != NULL) {
        return _BinkDoFrame(bnk);
    } else {
        return -1;
    }
}

void BINKCALL BinkNextFrame(HBINK bnk)
{
    if (_BinkNextFrame != NULL) {
        _BinkNextFrame(bnk);
    }
}

HBINK BINKCALL BinkOpen(const char* name, unsigned flags)
{
    if (_BinkOpen != NULL) {
        return _BinkOpen(name, flags);
    } else {
        return NULL;
    }
}

BINKSNDOPEN BINKCALL BinkOpenMiles(void* param)
{
    if (_BinkOpenMiles != NULL) {
        return _BinkOpenMiles(param);
    } else {
        return NULL;
    }
}

int BINKCALL BinkSetSoundSystem(BINKSNDSYSOPEN open, void* param)
{
    if (_BinkSetSoundSystem != NULL) {
        return _BinkSetSoundSystem(open, param);
    } else {
        return -1;
    }
}

void BINKCALL BinkSetSoundTrack(unsigned track)
{
    if (_BinkSetSoundTrack != NULL) {
        _BinkSetSoundTrack(track);
    }
}

int BINKCALL BinkWait(HBINK bnk)
{
    if (_BinkWait != NULL) {
        return _BinkWait(bnk);
    } else {
        return -1;
    }
}

bool bink_compat_init(void)
{
#ifdef _WIN32
    static const char* bink_library_names[] = {
#if defined(_WIN64)
        "binkw64.dll",
        "bink2w64.dll",
#else
        "binkw32.dll",
#endif
    };
    size_t index;

    for (index = 0; index < sizeof(bink_library_names) / sizeof(bink_library_names[0]); index++) {
        bink_library = LoadLibraryA(bink_library_names[index]);
        if (bink_library != NULL) {
            break;
        }
    }

    if (bink_library == NULL) {
        return false;
    }

    _BinkClose = (BINKCLOSE)bink_compat_get_proc("_BinkClose@4", "BinkClose");
    _BinkCopyToBuffer = (BINKCOPYTOBUFFER)bink_compat_get_proc("_BinkCopyToBuffer@28", "BinkCopyToBuffer");
    _BinkDDSurfaceType = (BINKDDSURFACETYPE)bink_compat_get_proc("_BinkDDSurfaceType@4", "BinkDDSurfaceType");
    _BinkDoFrame = (BINKDOFRAME)bink_compat_get_proc("_BinkDoFrame@4", "BinkDoFrame");
    _BinkNextFrame = (BINKNEXTFRAME)bink_compat_get_proc("_BinkNextFrame@4", "BinkNextFrame");
    _BinkOpen = (BINKOPEN)bink_compat_get_proc("_BinkOpen@8", "BinkOpen");
    _BinkOpenMiles = (BINKOPENMILES)bink_compat_get_proc("_BinkOpenMiles@4", "BinkOpenMiles");
    _BinkSetSoundSystem = (BINKSETSOUNDSYSTEM)bink_compat_get_proc("_BinkSetSoundSystem@8", "BinkSetSoundSystem");
    _BinkSetSoundTrack = (BINKSETSOUNDTRACK)bink_compat_get_proc("_BinkSetSoundTrack@4", "BinkSetSoundTrack");
    _BinkWait = (BINKWAIT)bink_compat_get_proc("_BinkWait@4", "BinkWait");

    if (_BinkClose == NULL
        || _BinkCopyToBuffer == NULL
        || _BinkDoFrame == NULL
        || _BinkNextFrame == NULL
        || _BinkOpen == NULL
        || _BinkSetSoundSystem == NULL
        || _BinkSetSoundTrack == NULL
        || _BinkWait == NULL) {
        bink_compat_exit();
        return false;
    }
#endif

    return true;
}

void bink_compat_exit(void)
{
#ifdef _WIN32
    if (bink_library != NULL) {
        FreeLibrary(bink_library);
        bink_library = NULL;

        _BinkClose = NULL;
        _BinkCopyToBuffer = NULL;
        _BinkDDSurfaceType = NULL;
        _BinkDoFrame = NULL;
        _BinkNextFrame = NULL;
        _BinkOpen = NULL;
        _BinkOpenMiles = NULL;
        _BinkSetSoundSystem = NULL;
        _BinkSetSoundTrack = NULL;
        _BinkWait = NULL;
    }
#endif
}

#ifdef _WIN32
static FARPROC bink_compat_get_proc(const char* decorated_name, const char* name)
{
    FARPROC proc = NULL;

    if (decorated_name != NULL) {
        proc = GetProcAddress(bink_library, decorated_name);
    }

    if (proc == NULL && name != NULL) {
        proc = GetProcAddress(bink_library, name);
    }

    return proc;
}
#endif
