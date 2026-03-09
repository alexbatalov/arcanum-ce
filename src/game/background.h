#ifndef ARCANUM_GAME_BACKGROUND_H_
#define ARCANUM_GAME_BACKGROUND_H_

#include "game/context.h"
#include "game/obj.h"

#define BACKGROUND_EDUCATOR 51
#define BACKGROUND_IDIOT_SAVANT 55
#define BACKGROUND_DAY_MAGE 62
#define BACKGROUND_NIGHT_MAGE 63
#define BACKGROUND_SKY_MAGE 64
#define BACKGROUND_NATURE_MAGE 65
#define BACKGROUND_AGORAPHOBIC 66
#define BACKGROUND_HYDROPHOBIC 67
#define BACKGROUND_AFRAID_OF_THE_DARK 68
#define BACKGROUND_MAGICK_ALLERGY 69
#define BACKGROUND_TECHNOPHOBIA 72
#define BACKGROUND_FRANKENSTEIN_MONSTER 74
#define BACKGROUND_BRIDE_OF_FRANKENSTEIN 75
#define BACKGROUND_DARK_SIGHT 78

bool background_init(GameInitInfo* init_info);
void background_exit(void);
bool background_find_first(int64_t obj, int* background_ptr);
bool background_find_next(int64_t obj, int* background_ptr);
bool background_find_prev(int64_t obj, int* background_ptr);
int background_get_description(int background);
char* background_description_get_text(int num);
char* background_description_get_body(int num);
char* background_description_get_name(int num);
void background_set(int64_t obj, int background, int background_text);
void background_clear(int64_t obj);
int background_get(int64_t obj);
int background_text_get(int64_t obj);
void background_generate_inventory(int64_t obj);
void background_educate_followers(int64_t obj);
int background_adjust_money(int amount, int background);
bool background_get_items(char* dest, size_t size, int background);

#endif /* ARCANUM_GAME_BACKGROUND_H_ */
