#ifndef ARCANUM_UI_TEXTEDIT_UI_H_
#define ARCANUM_UI_TEXTEDIT_UI_H_

#include "game/context.h"

typedef unsigned int TextEditFlags;

#define TEXTEDIT_PATH_SAFE 0x01
#define TEXTEDIT_NO_ALPHA 0x02

struct TextEdit;

typedef void(TextEditCallback)(struct TextEdit* textedit);

typedef struct TextEdit {
    /* 0000 */ TextEditFlags flags;
    /* 0004 */ char* buffer;
    /* 0008 */ int size;
    /* 000C */ TextEditCallback* on_enter;
    /* 0010 */ TextEditCallback* on_change;
    /* 0014 */ TextEditCallback* on_tab;
} TextEdit;

bool textedit_ui_init(GameInitInfo* init_info);
void textedit_ui_reset(void);
void textedit_ui_exit(void);
void textedit_ui_focus(TextEdit* textedit);
void textedit_ui_unfocus(TextEdit* textedit);
bool textedit_ui_is_focused(void);
bool textedit_ui_process_message(TigMessage* msg);
void textedit_ui_clear(void);
void textedit_ui_restore(void);
void textedit_ui_submit(void);

#endif /* ARCANUM_UI_TEXTEDIT_UI_H_ */
