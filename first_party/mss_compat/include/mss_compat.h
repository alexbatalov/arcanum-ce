#ifndef MSS_COMPAT_H_
#define MSS_COMPAT_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define AILCALL __stdcall
#else
#define AILCALL
#endif

#define QSTAT_DONE 1
#define QSTAT_PLAYING 3

#define SMP_PLAYING 0x0004

typedef struct AUDIO* HAUDIO;
typedef void* HDIGDRIVER;
typedef void* HDLSDEVICE;
typedef void* HMDIDRIVER;
typedef struct STREAM* HSTREAM;

void AILCALL AIL_close_stream(HSTREAM stream);
int AILCALL AIL_digital_handle_reacquire(HDIGDRIVER drvr);
int AILCALL AIL_digital_handle_release(HDIGDRIVER drvr);
HSTREAM AILCALL AIL_open_stream(HDIGDRIVER dig, const char* filename, int stream_mem);
void AILCALL AIL_quick_handles(HDIGDRIVER* pdig, HMDIDRIVER* pmdi, HDLSDEVICE* pdls);
HAUDIO AILCALL AIL_quick_load_mem(void const* mem, unsigned size);
int AILCALL AIL_quick_play(HAUDIO audio, unsigned loop_count);
void AILCALL AIL_quick_set_volume(HAUDIO audio, int volume, int extravol);
void AILCALL AIL_quick_shutdown(void);
int AILCALL AIL_quick_startup(int use_digital, int use_MIDI, unsigned output_rate, int output_bits, int output_channels);
int AILCALL AIL_quick_status(HAUDIO audio);
void AILCALL AIL_quick_unload(HAUDIO audio);
void AILCALL AIL_set_stream_loop_count(HSTREAM stream, int count);
void AILCALL AIL_set_stream_volume(HSTREAM stream, int volume);
void AILCALL AIL_start_stream(HSTREAM stream);
int AILCALL AIL_stream_status(HSTREAM stream);

bool mss_compat_init();
void mss_compat_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* MSS_COMPAT_H_ */
