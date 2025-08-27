#ifndef BINK_COMPAT_H_
#define BINK_COMPAT_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define BINKCALL __stdcall
#else
#define BINKCALL
#endif

#define BINKSNDTRACK 0x4000

typedef unsigned char u8;
typedef signed long s32;
typedef unsigned long u32;

typedef struct BINK* HBINK;
typedef struct BINKSND BINKSND;

typedef s32(BINKCALL* BINKSNDOPEN)(struct BINKSND* BnkSnd, u32 freq, s32 bits, s32 chans, u32 flags, HBINK bink);
typedef s32(BINKCALL* BINKSNDREADY)(struct BINKSND* BnkSnd);
typedef s32(BINKCALL* BINKSNDLOCK)(struct BINKSND* BnkSnd, u8** addr, u32* len);
typedef s32(BINKCALL* BINKSNDUNLOCK)(struct BINKSND* BnkSnd, u32 filled);
typedef void(BINKCALL* BINKSNDVOLUME)(struct BINKSND* BnkSnd, s32 volume);
typedef void(BINKCALL* BINKSNDPAN)(struct BINKSND* BnkSnd, s32 pan);
typedef s32(BINKCALL* BINKSNDONOFF)(struct BINKSND* BnkSnd, s32 status);
typedef s32(BINKCALL* BINKSNDPAUSE)(struct BINKSND* BnkSnd, s32 status);
typedef void(BINKCALL* BINKSNDCLOSE)(struct BINKSND* BnkSnd);

typedef BINKSNDOPEN(BINKCALL* BINKSNDSYSOPEN)(void* param);

typedef struct BINKSND {
    BINKSNDREADY Ready;
    BINKSNDLOCK Lock;
    BINKSNDUNLOCK Unlock;
    BINKSNDVOLUME Volume;
    BINKSNDPAN Pan;
    BINKSNDPAUSE Pause;
    BINKSNDONOFF SetOnOff;
    BINKSNDCLOSE Close;
    u32 BestSizeIn16;
    u32 SoundDroppedOut;
    s32 OnOff;
    u32 Latency;
    u32 freq;
    s32 bits, chans;
    u8 snddata[128];
} BINKSND;

typedef struct BINK {
    unsigned Width;
    unsigned Height;
    unsigned Frames;
    unsigned FrameNum;
} BINK;

void BINKCALL BinkClose(HBINK bnk);
int BINKCALL BinkCopyToBuffer(HBINK bnk, void* dest, int destpitch, unsigned destheight, unsigned destx, unsigned desty, unsigned flags);
int BINKCALL BinkDDSurfaceType(void* lpDDS);
int BINKCALL BinkDoFrame(HBINK bnk);
void BINKCALL BinkNextFrame(HBINK bnk);
HBINK BINKCALL BinkOpen(const char* name, unsigned flags);
BINKSNDOPEN BINKCALL BinkOpenMiles(void* param);
int BINKCALL BinkSetSoundSystem(BINKSNDSYSOPEN open, void* param);
void BINKCALL BinkSetSoundTrack(unsigned track);
int BINKCALL BinkWait(HBINK bnk);

bool bink_compat_init();
void bink_compat_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* BINK_COMPAT_H_ */
