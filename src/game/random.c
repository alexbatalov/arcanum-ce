#include "game/random.h"

#include <time.h>

/**
 * 0x601778
 */
static int random_prev_value;

static void random_set_prev_value(int value);

/**
 * Called when the game is initialized.
 *
 * 0x4CEEB0
 */
bool random_init(GameInitInfo* init_info)
{
    (void)init_info;

    random_seed_generate();

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4CEEC0
 */
void random_exit(void)
{
}

/**
 * Sets the seed for the random number generator.
 *
 * 0x4CEED0
 */
void random_seed(int value)
{
    random_set_prev_value(value);
}

/**
 * Generates a seed based on the current time and sets it.
 *
 * 0x4CEEE0
 */
int random_seed_generate(void)
{
    int seed = (int)time(NULL);

    random_seed(seed);

    return seed;
}

/**
 * Generates a random integer within a specified inclusive range.
 *
 * 0x4CEF00
 */
int random_between(int lower, int upper)
{
    if (lower == upper) {
        return lower;
    } else if (lower < upper) {
        return lower + random_rand() % (upper - lower + 1);
    } else {
        return upper + random_rand() % (lower - upper + 1);
    }
}

/**
 * Sets the internal state of the random number generator.
 *
 * 0x4CEF40
 */
void random_set_prev_value(int value)
{
    random_prev_value = value;
}

/**
 * Generates the next random number.
 *
 * NOTE: The parameters are from Knuth and H.W. Lewis, Numerical Recipes, 2nd.
 * edition, chapter 7.1, $An Even Quicker Generator, p.284, available for free
 * on their website.
 *
 * The Numerical Recipes, 3rd. edition no longer recommends using LCG. I guess
 * that CE can follow ToEE approach, which has a switch to select either this
 * legacy implementation (referred to as `arcanum` in ToEE codebase), or
 * Mersenne Twister (`mt19937ar`), with the latter being the default.
 *
 * See:
 *  - https://numerical.recipes
 *  - https://en.wikipedia.org/wiki/Linear_congruential_generator
 *  - https://en.wikipedia.org/wiki/Mersenne_Twister
 *
 * 0x4CEF50
 */
int random_rand(void)
{
    random_prev_value = 1664525 * random_prev_value + 1013904223;
    return (random_prev_value >> 8) & 0x7FFF;
}
