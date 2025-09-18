#include "ui/textedit_ui.h"

static bool textedit_ui_validate(char ch);

/**
 * Flag indicating whether a text edit control is currently focused.
 *
 * 0x66DAA4
 */
static int textedit_ui_focused;

/**
 * Buffer to store the initial value of the text edit control.
 *
 * NOTE: Odd buffer size.
 *
 * 0x66D9D8
 */
static char textedit_ui_backup[204];

/**
 * Pointer to the currently focused text edit control.
 *
 * 0x66DAA8
 */
static TextEdit* textedit_ui_current_textedit;

/**
 * Current cursor position within the text edit buffer.
 *
 * 0x66DAAC
 */
static int textedit_ui_pos;

/**
 * Flag indicating whether a text edit control in overwrite mode.
 *
 * 0x66DAB0
 */
static int textedit_ui_overwrite_mode;

/**
 * Length of the text in the current text edit buffer.
 *
 * 0x66DAB
 */
static int textedit_ui_len;

/**
 * Called when the game is initialized.
 *
 * 0x566E20
 */
bool textedit_ui_init(GameInitInfo* init_info)
{
    (void)init_info;

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x566E30
 */
void textedit_ui_reset()
{
}

/**
 * Called when the game shuts down.
 *
 * 0x566E40
 */
void textedit_ui_exit()
{
}

/**
 * Sets focus to a specified text edit control.
 *
 * 0x566E50
 */
void textedit_ui_focus(TextEdit* textedit)
{
    SDL_Window* window;

    if (tig_video_window_get(&window) == TIG_OK) {
        SDL_StartTextInput(window);
    }

    textedit_ui_current_textedit = textedit;

    if (*textedit->buffer != '\0') {
        textedit_ui_pos = strlen(textedit->buffer);
        textedit_ui_len = textedit_ui_pos;
    } else {
        textedit_ui_pos = 0;
        textedit_ui_len = 0;
    }

    textedit_ui_overwrite_mode = 0;

    strcpy(textedit_ui_backup, textedit->buffer);
    textedit_ui_focused = true;
}

/**
 * Removes focus from a specified text edit control.
 *
 * 0x566ED0
 */
void textedit_ui_unfocus(TextEdit* textedit)
{
    SDL_Window* window;

    (void)textedit;

    if (tig_video_window_get(&window) == TIG_OK) {
        SDL_StopTextInput(window);
    }

    textedit_ui_current_textedit = NULL;
    textedit_ui_pos = 0;
    textedit_ui_focused = false;
}

/**
 * Checks if a text edit control is currently focused.
 *
 * 0x566EF0
 */
bool textedit_ui_is_focused()
{
    return textedit_ui_focused;
}

/**
 * Processes input for the focused text edit control.
 *
 * 0x566F00
 */
bool textedit_ui_process_message(TigMessage* msg)
{
    if (textedit_ui_current_textedit == NULL) {
        return false;
    }

    if (msg->type == TIG_MESSAGE_KEYBOARD) {
        if (msg->data.keyboard.pressed) {
            switch (msg->data.keyboard.key) {
            case SDLK_HOME:
                // Move cursor to beginning of the string.
                textedit_ui_pos = 0;
                break;
            case SDLK_UP:
                // Move cursor one "page" left.
                if (textedit_ui_pos - 40 >= 0) {
                    textedit_ui_pos -= 40;
                }
                break;
            case SDLK_LEFT:
                // Move cursor left.
                if (textedit_ui_pos > 0) {
                    textedit_ui_pos--;
                }
                break;
            case SDLK_RIGHT:
                // Move cursor right.
                if (textedit_ui_pos < textedit_ui_len) {
                    textedit_ui_pos++;
                }
                break;
            case SDLK_DOWN:
                // Move cursor one "page" right.
                if (textedit_ui_pos + 40 < textedit_ui_len) {
                    textedit_ui_pos += 40;
                }
                break;
            case SDLK_INSERT:
                // Toggle insert/overwrite mode.
                textedit_ui_overwrite_mode = !textedit_ui_overwrite_mode;
                break;
            case SDLK_DELETE:
                // Delete character after cursor.
                if (textedit_ui_pos < textedit_ui_len) {
                    memmove(&(textedit_ui_current_textedit->buffer[textedit_ui_pos]),
                        &(textedit_ui_current_textedit->buffer[textedit_ui_pos + 1]),
                        textedit_ui_len - textedit_ui_pos);
                    textedit_ui_current_textedit->buffer[textedit_ui_len--] = '\0';
                }
                break;
            case SDLK_BACKSPACE:
                // Delete character before cursor.
                if (textedit_ui_pos > 0) {
                    textedit_ui_pos--;
                    memmove(&(textedit_ui_current_textedit->buffer[textedit_ui_pos]),
                        &(textedit_ui_current_textedit->buffer[textedit_ui_pos + 1]),
                        textedit_ui_len - textedit_ui_pos);
                    textedit_ui_current_textedit->buffer[textedit_ui_len--] = '\0';
                }
                break;
            case SDLK_TAB:
                // Handle tab key with callback.
                if (textedit_ui_current_textedit->on_tab != NULL) {
                    textedit_ui_current_textedit->on_tab(textedit_ui_current_textedit);
                }
                return true;
            case SDLK_RETURN:
                // Handle enter key with callback.
                textedit_ui_current_textedit->on_enter(textedit_ui_current_textedit);
                return true;
            default:
                return false;
            }

            // Trigger the change callback.
            if (textedit_ui_current_textedit->on_change != NULL) {
                textedit_ui_current_textedit->on_change(textedit_ui_current_textedit);
            }

            return true;
        }

        return false;
    }

    if (msg->type == TIG_MESSAGE_TEXT_INPUT) {
        const char* pch = msg->data.text.text;
        while (*pch != '\0') {
            // Validate character input.
            if ((*pch >= '\0' && *pch < ' ')
                || textedit_ui_pos >= textedit_ui_current_textedit->size - 1
                || !textedit_ui_validate(*pch)) {
                break;
            }

            if (!textedit_ui_overwrite_mode) {
                // Check max string size.
                if (textedit_ui_len > textedit_ui_current_textedit->size - 1) {
                    return true;
                }

                textedit_ui_len++;

                // Shift characters to the right to make a room a new character.
                memmove(&(textedit_ui_current_textedit->buffer[textedit_ui_pos + 1]),
                    &(textedit_ui_current_textedit->buffer[textedit_ui_pos]),
                    textedit_ui_len - textedit_ui_pos);
            }

            // Write character at cursor.
            textedit_ui_current_textedit->buffer[textedit_ui_pos] = *pch;

            // Update cursor and string length.
            if (++textedit_ui_pos > textedit_ui_len) {
                textedit_ui_len = textedit_ui_pos;
                textedit_ui_current_textedit->buffer[textedit_ui_pos] = '\0';
            }

            pch++;
        }

        // Trigger the change callback.
        if (textedit_ui_current_textedit->on_change != NULL) {
            textedit_ui_current_textedit->on_change(textedit_ui_current_textedit);
        }

        return true;
    }

    return false;
}

/**
 * Validates a character based on text edit control flags.
 *
 * 0x5671E0
 */
bool textedit_ui_validate(char ch)
{
    if ((textedit_ui_current_textedit->flags & TEXTEDIT_PATH_SAFE) != 0) {
        switch (ch) {
        case '"':
        case '\'':
        case '*':
        case '/':
        case ':':
        case '<':
        case '>':
        case '?':
        case '\\':
        case '|':
            return false;
        }
    }

    if ((textedit_ui_current_textedit->flags & TEXTEDIT_NO_ALPHA) != 0) {
        if (ch >= 'A' && ch <= 'Z') {
            return false;
        }

        if (ch >= 'a' && ch <= 'z') {
            return false;
        }
    }

    return true;
}

/**
 * Clears the content of the focused text edit control.
 *
 * 0x5672A0
 */
void textedit_ui_clear()
{
    textedit_ui_pos = 0;
    textedit_ui_len = 0;

    if (textedit_ui_current_textedit != NULL) {
        *textedit_ui_current_textedit->buffer = '\0';
        if (textedit_ui_current_textedit != NULL) {
            textedit_ui_current_textedit->on_change(textedit_ui_current_textedit);
        }
    }
}

/**
 * Restores the content of the focused text edit control from the backup buffer.
 *
 * 0x5672D0
 */
void textedit_ui_restore()
{
    if (textedit_ui_current_textedit != NULL) {
        strcpy(textedit_ui_current_textedit->buffer, textedit_ui_backup);
        if (textedit_ui_current_textedit != NULL) {
            textedit_ui_current_textedit->on_change(textedit_ui_current_textedit);
        }
    }
}

/**
 * Submits the content of the focused text edit control.
 *
 * 0x567320
 */
void textedit_ui_submit()
{
    if (textedit_ui_current_textedit != NULL) {
        textedit_ui_current_textedit->on_enter(textedit_ui_current_textedit);
    }
}
