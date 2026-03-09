#include "game/hrp.h"

static int hrp_iso_window_width;
static int hrp_iso_window_height;

bool hrp_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    hrp_iso_window_width = window_data.rect.width;
    hrp_iso_window_height = window_data.rect.height;

    return true;
}

void hrp_resize(GameResizeInfo* resize_info)
{
    hrp_iso_window_width = resize_info->window_rect.width;
    hrp_iso_window_height = resize_info->window_rect.height;
}

void hrp_apply(TigRect* rect, Gravity gravity)
{
    int dx;
    int dy;

    if ((gravity & GRAVITY_LEFT) != 0) {
        dx = 0;
    } else if ((gravity & GRAVITY_RIGHT) != 0) {
        dx = hrp_iso_window_width - 800;
    } else if ((gravity & GRAVITY_CENTER_HORIZONTAL) != 0) {
        dx = (hrp_iso_window_width - 800) / 2;
    }

    if ((gravity & GRAVITY_TOP) != 0) {
        dy = 0;
    } else if ((gravity & GRAVITY_BOTTOM) != 0) {
        dy = hrp_iso_window_height - 600;
    } else if ((gravity & GRAVITY_CENTER_VERTICAL) != 0) {
        dy = (hrp_iso_window_height - 600) / 2;
    }

    rect->x += dx;
    rect->y += dy;
}

void hrp_center(int* x, int* y)
{
    *x += (hrp_iso_window_width - 800) / 2;
    *y += (hrp_iso_window_height - 600) / 2;
}

int hrp_iso_window_width_get(void)
{
    return hrp_iso_window_width;
}

int hrp_iso_window_height_get(void)
{
    return hrp_iso_window_height;
}
