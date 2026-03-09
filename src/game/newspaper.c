#include "game/newspaper.h"

#include "game/descriptions.h"
#include "game/mes.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/proto.h"
#include "game/random.h"
#include "game/ui.h"

/**
 * The maximum number of enqueued newspapers.
 */
#define QUEUE_SIZE 25

/**
 * The base number of generic newspapers.
 *
 * This value is used to separate story-specific (below this number) vs. generic
 * newspapers (greater or equal).
 */
#define GENERIC_NEWSPAPER_NUM 5000

static bool is_in_queue(int num);
static void rotate_newspapers(void);
static int pick_random_generic_newspaper(void);

/**
 * Current number of newspapers in the queue.
 *
 * 0x6876CC
 */
static int newspaper_queue_size;

/**
 * Array storing the numbers of queued newspapers.
 *
 * The queue has a special element at index 4 (`NEWSPAPER_CURRENT`), which
 * denotes today's newspaper. This special index separates queue into two
 * logical sections: indexes 0-3 represents "past" newspapers, and indexes 5-24
 * represents "future" newspapers.
 *
 * The "past" section can only contain story-specific newspapers. The newspaper
 * at index 3 is the previous story-specific newspaper. The newspaper at index 0
 * is the oldest story-specific newspaper.
 *
 * The "future" section can contain both story-specific and generic newspapers.
 * The newspaper at index 5 is tomorrow's newspaper.
 *
 * 0x6876D0
 */
static int* newspaper_queue;

/**
 * Called when the game is initialized.
 *
 * 0x4BF060
 */
bool newspaper_init(GameInitInfo* init_info)
{
    (void)init_info;

    newspaper_queue = (int*)CALLOC(QUEUE_SIZE, sizeof(*newspaper_queue));
    newspaper_reset();

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4BF080
 */
void newspaper_reset(void)
{
    int index;

    // Invalidate "past" newspapers.
    for (index = 0; index < NEWSPAPER_CURRENT; index++) {
        newspaper_queue[index] = -1;
    }

    newspaper_queue[NEWSPAPER_CURRENT] = GENERIC_NEWSPAPER_NUM;
    newspaper_queue_size = NEWSPAPER_CURRENT + 1;
}

/**
 * Called when the game shuts down.
 *
 * 0x4BF0B0
 */
void newspaper_exit(void)
{
    FREE(newspaper_queue);
}

/**
 * Called when the game is being loaded.
 *
 * 0x4BF0C0
 */
bool newspaper_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(newspaper_queue, sizeof(*newspaper_queue) * QUEUE_SIZE, 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&newspaper_queue_size, sizeof(newspaper_queue_size), 1, load_info->stream) != 1) return false;

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x4BF100
 */
bool newspaper_save(TigFile* stream)
{
    if (tig_file_fwrite(newspaper_queue, sizeof(*newspaper_queue) * QUEUE_SIZE, 1, stream) != 1) return false;
    if (tig_file_fwrite(&newspaper_queue_size, sizeof(newspaper_queue_size), 1, stream) != 1) return false;

    return true;
}

/**
 * Adds a newspaper to the queue.
 *
 * When `priority` is `true` the newspaper is added at the front of the queue,
 * thus becoming tomorrow's issue. Otherwise it's just appended to the end of
 * the queue.
 *
 * 0x4BF140
 */
void newspaper_enqueue(int num, bool priority)
{
    int index;

    // Skip adding if trying to add story-specific newspaper which is already
    // in the queue (thus avoiding duplicates).
    if (num < GENERIC_NEWSPAPER_NUM && is_in_queue(num)) {
        return;
    }

    if (priority) {
        // Shift elements to make room for the new newspaper at the front.
        for (index = QUEUE_SIZE - 1; index > NEWSPAPER_CURRENT; index--) {
            newspaper_queue[index] = newspaper_queue[index - 1];
        }

        // Make the specified newspaper next in the queue.
        newspaper_queue[NEWSPAPER_CURRENT + 1] = num;

        if (newspaper_queue_size < QUEUE_SIZE) {
            newspaper_queue_size++;
        }
    } else {
        // Append to the end of the queue or overwrite the last entry if queue
        // is full.
        if (newspaper_queue_size < QUEUE_SIZE) {
            newspaper_queue[newspaper_queue_size++] = num;
        } else {
            newspaper_queue[QUEUE_SIZE - 1] = num;
        }
    }
}

/**
 * Checks if a newspaper is in the queue.
 *
 * 0x4BF1D0
 */
bool is_in_queue(int num)
{
    int index;

    for (index = 0; index < QUEUE_SIZE; index++) {
        if (newspaper_queue[index] == num) {
            return true;
        }
    }

    return false;
}

/**
 * Retrieves a newspaper from the queue by index.
 *
 * This function is used to obtain today's newspaper (indexed by
 * `NEWSPAPER_CURRENT`) or previous newspapers (indexes 0-3).
 *
 * 0x4BF200
 */
int newspaper_get(int index)
{
    return newspaper_queue[index];
}

/**
 * Creates a newspaper game object at a specified location.
 *
 * 0x4BF210
 */
int64_t newspaper_create(int num, int64_t loc)
{
    int64_t prototype_obj;
    int64_t obj;

    // Retrieve "Tarant Newspaper" prototype.
    prototype_obj = sub_4685A0(BP_TARANT_NEWSPAPER);

    // Create the newspaper object at the specified location.
    if (!object_create(prototype_obj, loc, &obj)) {
        return OBJ_HANDLE_NULL;
    }

    // Store the newspaper number. See `writte_ui.c` for usage.
    obj_field_int32_set(obj, OBJ_F_WRITTEN_TEXT_START_LINE, num);

    return obj;
}

/**
 * Called by timeevent scheduler.
 *
 * 0x4BF270.
 */
bool newspaper_timeevent_process(TimeEvent* timeevent)
{
    TimeEvent next_timeevent;
    DateTime datetime;

    (void)timeevent;

    if (tig_net_is_active() && !tig_net_is_host()) {
        return true;
    }

    rotate_newspapers();

    // Schedule the next rotation event in 24 hours.
    next_timeevent.type = TIMEEVENT_TYPE_NEWSPAPERS;
    sub_45A950(&datetime, 86400000);
    timeevent_add_delay(&next_timeevent, &datetime);

    return true;
}

/**
 * Rotates the newspaper queue by removing the oldest entry and adding a new
 * generic newspaper if needed.
 *
 * 0x4BF2C0
 */
void rotate_newspapers(void)
{
    int index;

    // When today's newspaper is story-specific, the starting index for rotation
    // is zero, which denotes the oldest story-specific newspaper. In this case
    // entire queue will be shifted up by one element.
    //
    // Otherwise, when today's newspaper is generic, we want to keep old
    // story-specific newspapers and shift only the "future" portion of the
    // queue.
    index = newspaper_queue[NEWSPAPER_CURRENT] < GENERIC_NEWSPAPER_NUM ? 0 : NEWSPAPER_CURRENT;

    // Decrease queue size and shift entries up to remove the oldest newspaper.
    newspaper_queue_size--;
    while (index < newspaper_queue_size) {
        newspaper_queue[index] = newspaper_queue[index + 1];
        index++;
    }

    // Add a new generic newspaper when the queue does not have enough old
    // story-specific newspapers. This only happens early the game until the
    // "past" portion of the queue is filled with story-specific newspapers.
    if (newspaper_queue_size < 5) {
        newspaper_queue[NEWSPAPER_CURRENT] = pick_random_generic_newspaper();
        newspaper_queue_size = NEWSPAPER_CURRENT + 1;
    }
}

/**
 * Selects a random generic newspaper number.
 *
 * 0x4BF330
 */
int pick_random_generic_newspaper(void)
{
    int cnt = 0;
    int num = GENERIC_NEWSPAPER_NUM;
    char str[MAX_STRING];

    // Count valid newspaper headlines until an empty string is found.
    //
    // The `gamenewspaper.mes` description says that "...new newspapers begins
    // on a multiple of 10...".
    do {
        cnt++;
        num += 10;
        ui_written_newspaper_headline(num, str);
    } while (str[0] != '\0');

    // Select a random number from the valid range.
    return GENERIC_NEWSPAPER_NUM + random_between(0, cnt - 1) * 10;
}
