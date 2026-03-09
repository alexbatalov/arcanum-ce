#ifndef ARCANUM_GAME_TECH_H_
#define ARCANUM_GAME_TECH_H_

#include "game/context.h"

typedef enum Tech {
    TECH_HERBOLOGY,
    TECH_CHEMISTRY,
    TECH_ELECTRIC,
    TECH_EXPLOSIVES,
    TECH_GUN,
    TECH_MECHANICAL,
    TECH_SMITHY,
    TECH_THERAPEUTICS,
    TECH_COUNT,
} Tech;

typedef enum Degree {
    DEGREE_LAYMAN,
    DEGREE_NOVICE,
    DEGREE_ASSISTANT,
    DEGREE_ASSOCIATE,
    DEGREE_TECHNICIAN,
    DEGREE_ENGINEER,
    DEGREE_PROFESSOR,
    DEGREE_DOCTORATE,
    DEGREE_COUNT,
} Degree;

bool tech_init(GameInitInfo* init_info);
void tech_exit(void);
void tech_set_defaults(int64_t obj);
char* tech_discipline_name_get(int tech);
char* tech_discipline_description_get(int tech);
char* tech_degree_name_get(int degree);
char* tech_degree_description_get(int degree, int tech);
int tech_degree_get(int64_t obj, int tech);
int tech_degree_inc(int64_t obj, int tech);
int tech_degree_dec(int64_t obj, int tech);
int tech_degree_cost_get(int degree);
int tech_degree_level_get(int64_t obj, int tech);
int tech_degree_min_intelligence_get(int degree);
void tech_learn_schematic(int64_t pc_obj, int64_t written_obj);
bool tech_check_intelligence(int64_t obj, int intelligence);
int tech_schematic_get(int tech, int schematic);

#endif /* ARCANUM_GAME_TECH_H_ */
