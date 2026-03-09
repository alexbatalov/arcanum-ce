#ifndef ARCANUM_UI_SCHEMATIC_UI_H_
#define ARCANUM_UI_SCHEMATIC_UI_H_

#include "game/context.h"
#include "game/schematic.h"

bool schematic_ui_init(GameInitInfo* init_info);
void schematic_ui_exit(void);
void schematic_ui_reset(void);
void schematic_ui_toggle(int64_t primary_obj, int64_t secondary_obj);
void schematic_ui_close(void);
void schematic_ui_info_get(int schematic, SchematicInfo* schematic_info);
bool schematic_ui_process(int schematic, int64_t primary_obj, int64_t secondary_obj);
bool schematic_ui_feedback(bool success, int64_t primary_obj, int64_t secondary_obj);
char* schematic_ui_product_name(int schematic);
char* schematic_ui_learned_schematic_product_name(int tech, int degree);
void schematic_ui_learned_schematic_component_names(int tech, int degree, char* item1, char* item2);

#endif /* ARCANUM_UI_SCHEMATIC_UI_H_ */
