#ifndef ARCANUM_GAME_CONTEXT_H_
#define ARCANUM_GAME_CONTEXT_H_

#include <tig/tig.h>

#include "net_compat.h"

#define GAME 0
#define EDITOR 1

typedef enum ViewType {
    VIEW_TYPE_ISOMETRIC,
    VIEW_TYPE_TOP_DOWN,
} ViewType;

typedef void(IsoInvalidateRectFunc)(TigRect* rect);
typedef bool(IsoRedrawFunc)(void);

typedef struct GameInitInfo {
    bool editor;
    tig_window_handle_t iso_window_handle;
    IsoInvalidateRectFunc* invalidate_rect_func;
    IsoRedrawFunc* draw_func;
} GameInitInfo;

typedef struct GameResizeInfo {
    tig_window_handle_t window_handle;
    TigRect window_rect;
    TigRect content_rect;
} GameResizeInfo;

typedef struct GameLoadInfo {
    int version;
    TigFile* stream;
} GameLoadInfo;

typedef struct ViewOptions {
    int type;
    int zoom;
} ViewOptions;

typedef struct LocRect LocRect;

typedef struct SectorListNode {
    /* 0000 */ int64_t sec;
    /* 0008 */ int64_t loc;
    /* 0010 */ int width;
    /* 0014 */ int height;
    /* 0018 */ struct SectorListNode* next;
} SectorListNode;

// TODO: Better name.
typedef struct SomeSectorStuffEntry {
    /* 0000 */ int width;
    /* 0004 */ int field_4;
    /* 0008 */ int64_t field_8[3];
    /* 0020 */ int64_t field_20[3];
    /* 0038 */ int field_38[3];
    /* 0044 */ int field_44[3];
    /* 0050 */ int field_50;
    /* 0054 */ int field_54;
} SomeSectorStuffEntry;

// TODO: Better name.
typedef struct SomeSectorStuff {
    /* 0000 */ int height;
    /* 0004 */ int field_4;
    /* 0008 */ SomeSectorStuffEntry field_8[3];
} SomeSectorStuff;

typedef struct GameDrawInfo {
    TigRect* screen_rect;
    LocRect* loc_rect;
    SomeSectorStuff* field_8;
    SectorListNode* sectors;
    TigRectListNode** rects;
} GameDrawInfo;

typedef struct MapNewInfo {
    /* 0000 */ const char* base_path;
    /* 0004 */ const char* save_path;
    /* 0008 */ int base_terrain_type;
    /* 0010 */ int64_t width;
    /* 0018 */ int64_t height;
} MapNewInfo;

#endif /* ARCANUM_GAME_CONTEXT_H_ */
