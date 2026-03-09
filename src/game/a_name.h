#ifndef ARCANUM_GAME_A_NAME_H_
#define ARCANUM_GAME_A_NAME_H_

#include "game/obj.h"

typedef enum TileSoundType {
    TILE_SOUND_DIRT,
    TILE_SOUND_SAND,
    TILE_SOUND_SNOW,
    TILE_SOUND_STONE,
    TILE_SOUND_WATER,
    TILE_SOUND_WOOD,
    TILE_SOUND_COUNT,
} TileSoundType;

bool a_name_tile_init(void);
void a_name_tile_exit(void);
bool a_name_tile_aid_to_fname(tig_art_id_t aid, char* fname);
bool sub_4EBA30(tig_art_id_t a, tig_art_id_t b);
bool a_name_tile_is_blocking(tig_art_id_t aid);
bool a_name_tile_is_sinkable(tig_art_id_t aid);
bool a_name_tile_is_slippery(tig_art_id_t aid);
bool a_name_tile_is_natural(tig_art_id_t aid);
bool a_name_tile_is_soundproof(tig_art_id_t aid);
int a_name_tile_sound(tig_art_id_t aid);
void sub_4EBC40(void);
int sub_4EBEF0(int a1, int a2, int a3, int a4, int a5, int a6);
bool a_name_item_init(void);
void a_name_item_exit(void);
bool a_name_item_aid_to_fname(tig_art_id_t aid, char* fname);
bool a_name_facade_init(void);
void a_name_facade_exit(void);
bool a_name_facade_aid_to_fname(tig_art_id_t aid, char* fname);
bool a_name_portal_init(void);
void a_name_portal_exit(void);
bool a_name_portal_aid_to_fname(tig_art_id_t aid, char* fname);
tig_art_id_t a_name_portal_aid_from_wall_aid(tig_art_id_t wall_art_id, ObjectID* oid);
tig_art_id_t a_name_portal_aid_busted_set(tig_art_id_t aid);
bool a_name_wall_init(void);
void a_name_wall_exit(void);
bool a_name_wall_aid_to_fname(tig_art_id_t art_id, char* path);
tig_art_id_t a_name_wall_aid_damage_set(tig_art_id_t art_id, unsigned int flags);
int a_name_num_wall_structures(void);
char* a_name_wall_get_structure(int index);
bool a_name_light_init(void);
void a_name_light_exit(void);
bool a_name_light_aid_to_fname(tig_art_id_t aid, char* fname);
bool a_name_roof_init(void);
void a_name_roof_exit(void);
bool a_name_roof_aid_to_fname(tig_art_id_t aid, char* fname);
bool a_name_roof_fname_to_aid(const char* fname, tig_art_id_t* aid_ptr);

#endif /* ARCANUM_GAME_A_NAME_H_ */
