// RECT
// ---
//
// The RECT subsystem provides `TigRect` object, memory-efficient management of
// `TigRect` lists with `TigRectListNode` (intended for short-lived arbitrary
// long lists) and a set of utility geometry functions to deal with rectangles.
//
// NOTES
//
// - Unlike Fallouts where it's `Rect` was modelled after Windows' RECT (that is
// followed LTRB style), the `TigRect` uses LTWH style.

#include "tig/rect.h"

#include "tig/memory.h"

// Size of batch during node allocation.
#define GROW 20

#define MISS_BOTTOM 0x8
#define MISS_TOP 0x4
#define MISS_RIGHT 0x2
#define MISS_LEFT 0x1

static void tig_rect_node_reserve();
static void sub_52DC90(float x, float y, TigLine* line, unsigned int* flags);

// 0x62B2A4
static TigRectListNode* tig_rect_free_node_head;

// 0x52D0E0
int tig_rect_init(TigInitInfo* init_info)
{
    (void)init_info;

    return TIG_OK;
}

// 0x52D0F0
void tig_rect_exit()
{
    TigRectListNode* curr;
    TigRectListNode* next;

    curr = tig_rect_free_node_head;
    while (curr != NULL) {
        next = curr->next;
        FREE(curr);
        curr = next;
    }

    tig_rect_free_node_head = NULL;
}

// 0x52D120
TigRectListNode* tig_rect_node_create()
{
    TigRectListNode* node;

    node = tig_rect_free_node_head;
    if (node == NULL) {
        tig_rect_node_reserve();

        node = tig_rect_free_node_head;
        if (node == NULL) {
            return NULL;
        }
    }

    tig_rect_free_node_head = node->next;
    node->next = NULL;

    return node;
}

// 0x52D150
void tig_rect_node_reserve()
{
    int index;
    TigRectListNode* node;

    for (index = 0; index < GROW; index++) {
        node = (TigRectListNode*)MALLOC(sizeof(*node));
        if (node != NULL) {
            node->next = tig_rect_free_node_head;
            tig_rect_free_node_head = node;
        }
    }
}

// 0x52D180
void tig_rect_node_destroy(TigRectListNode* node)
{
    node->next = tig_rect_free_node_head;
    tig_rect_free_node_head = node;
}

// 0x52D1A0
int tig_rect_intersection(const TigRect* a, const TigRect* b, TigRect* r)
{
    int x;
    int y;
    int width;
    int height;

    if (a->x >= b->x) {
        x = a->x;
    } else {
        x = b->x;
    }

    if (a->y >= b->y) {
        y = a->y;
    } else {
        y = b->y;
    }

    if (a->width + a->x <= b->width + b->x) {
        width = a->x + a->width - x;
    } else {
        width = b->x + b->width - x;
    }

    if (width <= 0) {
        return TIG_ERR_NO_INTERSECTION;
    }

    if (a->height + a->y <= b->height + b->y) {
        height = a->y + a->height - y;
    } else {
        height = b->y + b->height - y;
    }

    if (height <= 0) {
        return TIG_ERR_NO_INTERSECTION;
    }

    r->x = x;
    r->y = y;
    r->width = width;
    r->height = height;

    return TIG_OK;
}

// 0x52D260
int tig_rect_clip(const TigRect* a, const TigRect* b, TigRect* rects)
{
    TigRect intersection;
    int count = 0;

    if (a->x >= b->x
        && a->y >= b->y
        && a->x + a->width <= b->x + b->width
        && a->y + a->height <= b->y + b->height) {
        return 0;
    }

    if (tig_rect_intersection(a, b, &intersection) == 4) {
        rects[0] = *a;
        return 1;
    }

    if (intersection.y > a->y) {
        rects[count].x = a->x;
        rects[count].y = a->y;
        rects[count].width = a->width;
        rects[count].height = intersection.y - a->y;
        count++;
    }

    if (intersection.x > a->x) {
        rects[count].x = a->x;
        rects[count].y = intersection.y;
        rects[count].width = intersection.x - a->x;
        rects[count].height = intersection.height;
        count++;
    }

    if (intersection.width + intersection.x < a->width + a->x) {
        rects[count].x = intersection.width + intersection.x;
        rects[count].y = intersection.y;
        rects[count].width = a->width + a->x - intersection.width - intersection.x;
        rects[count].height = intersection.height;
        count++;
    }

    if (intersection.height + intersection.y < a->height + a->y) {
        rects[count].x = a->x;
        rects[count].y = intersection.height + intersection.y;
        rects[count].width = a->width;
        rects[count].height = a->y + a->height - intersection.height - intersection.y;
        count++;
    }

    return count;
}

// 0x52D480
void sub_52D480(TigRectListNode** node_ptr, TigRect* rect)
{
    TigRectListNode* prev;
    TigRectListNode* node;
    TigRect tmp;
    int x;
    int y;
    int width;
    int height;
    unsigned int flags1;
    unsigned int flags2;
    unsigned int flags;
    int dx;
    int dy;

    x = rect->x;
    y = rect->y;
    width = rect->width;
    height = rect->height;

    prev = NULL;
    node = *node_ptr;

    while (node != NULL) {
        flags1 = 0;
        if (x >= node->rect.x
            && y >= node->rect.y
            && x < node->rect.x + node->rect.width
            && y < node->rect.y + node->rect.height) {
            flags1 |= 0x08;
        }

        if (x + width - 1 >= node->rect.x
            && y >= node->rect.y
            && x + width - 1 < node->rect.x + node->rect.width
            && y < node->rect.y + node->rect.height) {
            flags1 |= 0x04;
        }

        if (x + width - 1 >= node->rect.x
            && y + height - 1 >= node->rect.y
            && x + width - 1 < node->rect.x + node->rect.width
            && y + height - 1 < node->rect.y + node->rect.height) {
            flags1 |= 0x02;
        }

        if (x >= node->rect.x
            && y + height - 1 >= node->rect.y
            && x < node->rect.x + node->rect.width
            && y + height - 1 < node->rect.y + node->rect.height) {
            flags1 |= 0x01;
        }

        flags2 = 0;
        if (node->rect.x >= x
            && node->rect.y >= y
            && node->rect.x < x + width
            && node->rect.y < y + height) {
            flags2 |= 0x08;
        }

        if (node->rect.x + node->rect.width - 1 >= x
            && node->rect.y >= y
            && node->rect.x + node->rect.width - 1 < x + width
            && node->rect.y < y + height) {
            flags2 |= 0x04;
        }

        if (node->rect.x + node->rect.width - 1 >= x
            && node->rect.y + node->rect.height - 1 >= y
            && node->rect.x + node->rect.width - 1 < x + width
            && node->rect.y + node->rect.height - 1 < y + height) {
            flags2 |= 0x02;
        }

        if (node->rect.x >= x
            && node->rect.y + node->rect.height - 1 >= y
            && node->rect.x < x + width
            && node->rect.y + node->rect.height - 1 < y + height) {
            flags2 |= 0x01;
        }

        flags = flags1 | (flags2 << 4);
        switch (flags) {
        case 0x68:
        case 0x69:
        case 0x6C:
            node->rect.width = x - node->rect.x;

            prev = node;
            node = node->next;
            break;
        case 0x6F:
        case 0x8F:
        case 0x9F:
        case 0xCF:
        case 0xFF:
            return;
        case 0x82:
            tmp.x = x;
            tmp.y = y;
            tmp.width = width;
            tmp.height = node->rect.y - y;

            y += tmp.height;
            height -= tmp.height;
            width = node->rect.x + node->rect.width - x;

            node->rect.y += height;
            node->rect.height -= height;

            if (node->next != NULL) {
                sub_52D480(&(node->next), &tmp);
            } else {
                node->next = tig_rect_node_create();
                node->next->rect = tmp;
            }

            prev = node;
            node = node->next;
            break;
        case 0x83:
        case 0x93:
        case 0xC3:
            height = node->rect.y - y;

            prev = node;
            node = node->next;
            break;
        case 0x86:
        case 0x96:
        case 0xC6:
            width = node->rect.x - x;

            prev = node;
            node = node->next;
            break;
        case 0x90:
        case 0x92:
        case 0x94:
        case 0x9C:
            dx = x + width - node->rect.x;
            node->rect.x += dx;
            node->rect.width -= dx;

            prev = node;
            node = node->next;
            break;
        case 0xC0:
        case 0xC1:
        case 0xC2:
        case 0xC9:
            dy = y + height - node->rect.y;
            node->rect.y = y + height;
            node->rect.height -= dy;

            prev = node;
            node = node->next;
            break;
        case 0xF0:
        case 0xF1:
        case 0xF2:
        case 0xF3:
        case 0xF4:
        case 0xF6:
        case 0xF8:
        case 0xF9:
        case 0xFC:
            if (node->next == NULL) {
                node->rect.x = x;
                node->rect.y = y;
                node->rect.width = width;
                node->rect.height = height;
                return;
            }

            if (prev != NULL) {
                prev->next = node->next;
                tig_rect_node_destroy(node);
                node = prev->next;
            } else {
                *node_ptr = node->next;
                tig_rect_node_destroy(node);
                node = *node_ptr;
            }
            break;
        case 0x00:
            if (node->rect.x < x) {
                if (width + x >= node->rect.x + node->rect.width) {
                    prev = node;
                    node = node->next;
                    break;
                }

                if (y >= node->rect.y
                    || height + y <= node->rect.height + node->rect.y) {
                    prev = node;
                    node = node->next;
                    break;
                }

                tmp.x = x;
                tmp.width = width;
                tmp.y = node->rect.y + node->rect.height;
                tmp.height = y + height - tmp.y;
                height = node->rect.y - y;

                if (node->next != NULL) {
                    sub_52D480(&(node->next), &tmp);
                } else {
                    node->next = tig_rect_node_create();
                    node->next->rect = tmp;
                }

                prev = node;
                node = node->next;
            } else {
                if (node->rect.x <= x
                    || node->rect.x + node->rect.width >= width + x
                    || node->rect.y >= y
                    || node->rect.height + node->rect.y <= height + y) {
                    prev = node;
                    node = node->next;
                    break;
                }

                tmp.x = node->rect.x;
                tmp.width = node->rect.width;
                tmp.y = height + y;
                tmp.height = node->rect.y + node->rect.height - tmp.y;
                node->rect.height = y - node->rect.y;

                if (node->next != NULL) {
                    sub_52D480(&(node->next), &tmp);
                } else {
                    node->next = tig_rect_node_create();
                    node->next->rect = tmp;
                }

                prev = node;
                node = node->next;
            }
            break;
        case 0x03:
        case 0x43:
            height = node->rect.y - y;

            prev = node;
            node = node->next;
            break;
        case 0x06:
        case 0x16:
            width = node->rect.x - x;

            prev = node;
            node = node->next;
            break;
        case 0x09:
        case 0x29:
        case 0x49:
            dx = node->rect.x + node->rect.width - x;
            x = node->rect.x + node->rect.width;
            width -= dx;

            prev = node;
            node = node->next;
            break;
        case 0x0C:
        case 0x1C:
        case 0x2C:
            dy = node->rect.y + node->rect.height - y;
            y = node->rect.y + node->rect.height;
            height -= dy;

            prev = node;
            node = node->next;
            break;
        case 0x0F:
        case 0x1F:
        case 0x2F:
        case 0x3F:
        case 0x4F:
            return;
        case 0x14:
            tmp.x = x;
            tmp.y = node->rect.y + node->rect.height;
            tmp.width = width;
            tmp.height = y + height - tmp.y;

            height = tmp.y - y;
            width = node->rect.width + node->rect.x - x;
            node->rect.height = y - node->rect.y;

            if (node->next != NULL) {
                sub_52D480(&(node->next), &tmp);
            } else {
                node->next = tig_rect_node_create();
                node->next->rect = tmp;
            }

            prev = node;
            node = node->next;
            break;
        case 0x28:
            tmp.x = x;
            tmp.y = node->rect.y + node->rect.height;
            tmp.width = width;
            tmp.height = y + height - tmp.y;

            x = node->rect.x;
            width += tmp.x - node->rect.x;
            height = node->rect.y - y + node->rect.height;
            node->rect.height = y - node->rect.y;

            if (node->next != NULL) {
                sub_52D480(&(node->next), &tmp);
            } else {
                node->next = tig_rect_node_create();
                node->next->rect = tmp;
            }

            prev = node;
            node = node->next;
            break;
        case 0x30:
        case 0x31:
        case 0x34:
        case 0x36:
        case 0x38:
        case 0x39:
        case 0x3C:
            node->rect.height = y - node->rect.y;

            prev = node;
            node = node->next;
            break;
        case 0x41:
            tmp.x = x;
            tmp.y = y;
            tmp.width = width;
            tmp.height = node->rect.y - y;

            x = node->rect.x;
            y = node->rect.y;
            height -= tmp.height;
            width += tmp.x - node->rect.x;
            node->rect.y += height;
            node->rect.height -= height;

            if (node->next != NULL) {
                sub_52D480(&(node->next), &tmp);
            } else {
                node->next = tig_rect_node_create();
                node->next->rect = tmp;
            }

            prev = node;
            node = node->next;
            break;
        case 0x60:
        case 0x61:
            node->rect.width = x - node->rect.x;

            prev = node;
            node = node->next;
            break;
        case 0x63:
            height = node->rect.y - y;

            prev = node;
            node = node->next;
            break;
        default:
            prev = node;
            node = node->next;
            break;
        }
    }

    node = tig_rect_node_create();
    prev->next = node;
    node->rect.x = x;
    node->rect.y = y;
    node->rect.width = width;
    node->rect.height = height;
}

// 0x52DBB0
int tig_rect_union(const TigRect* a, const TigRect* b, TigRect* r)
{
    int x;
    int y;
    int width;
    int height;

    if (a->x >= b->x) {
        x = b->x;
    } else {
        x = a->x;
    }

    if (a->y >= b->y) {
        y = b->y;
    } else {
        y = a->y;
    }

    if (a->width + a->x <= b->width + b->x) {
        width = b->x + b->width - x;
    } else {
        width = a->x + a->width - x;
    }

    if (a->height + a->y <= b->height + b->y) {
        height = b->height + b->y - y;
    } else {
        height = a->height + a->y - y;
    }

    r->x = x;
    r->y = y;
    r->width = width;
    r->height = height;

    return TIG_OK;
}

// 0x52DC90
void sub_52DC90(float x, float y, TigLine* line, unsigned int* flags)
{
    *flags = 0;

    if (y > (float)line->y2) {
        *flags |= MISS_BOTTOM;
    } else if (y < (float)line->y1) {
        *flags |= MISS_TOP;
    }

    if (x > (float)line->x2) {
        *flags |= MISS_RIGHT;
    } else if (x < (float)line->x1) {
        *flags |= MISS_LEFT;
    }
}

// 0x52DD00
int tig_line_intersection(const TigRect* rect, TigLine* line)
{
    bool intersects = false;
    float x1, y1;
    float x2, y2;
    float x3, y3;
    TigLine temp_line;
    unsigned int flags1;
    unsigned int flags2;
    unsigned int flags;

    x1 = (float)line->x1;
    y1 = (float)line->y1;
    x2 = (float)line->x2;
    y2 = (float)line->y2;

    x3 = x2;
    y3 = y2;

    temp_line.x1 = rect->x;
    temp_line.y1 = rect->y;
    temp_line.x2 = rect->x + rect->width;
    temp_line.y2 = rect->y + rect->height;

    sub_52DC90(x1, y1, &temp_line, &flags1);
    sub_52DC90(x2, y2, &temp_line, &flags2);

    while (1) {
        if (flags1 == 0 && flags2 == 0) {
            intersects = true;
            break;
        }

        if (flags1 != 0 && flags2 != 0) {
            break;
        }

        flags = flags1 != 0 ? flags1 : flags2;

        if ((flags & MISS_BOTTOM) != 0) {
            y3 = (float)temp_line.y2;
            x3 = (y3 - y1) * (x2 - x1) / (y2 - y1) + x1;
        } else if ((flags & MISS_TOP) != 0) {
            y3 = (float)temp_line.y1;
            x3 = (y3 - y1) * (x2 - x1) / (y2 - y1) + x1;
        } else if ((flags & MISS_RIGHT) != 0) {
            x3 = (float)temp_line.x2;
            y3 = (x3 - x1) * (y2 - y1) / (x2 - x1) + y1;
        } else if ((flags & MISS_LEFT) != 0) {
            x3 = (float)temp_line.x1;
            y3 = (x3 - x1) * (y2 - y1) / (x2 - x1) + y1;
        }

        if (flags == flags1) {
            x1 = x3;
            y1 = y3;
            sub_52DC90(x3, y3, &temp_line, &flags1);
        } else {
            x2 = x3;
            y2 = y3;
            sub_52DC90(x3, y3, &temp_line, &flags2);
        }
    }

    line->x1 = (int)x1;
    line->y1 = (int)y1;
    line->x2 = (int)x2;
    line->y2 = (int)y2;

    if (!intersects) {
        return TIG_ERR_NO_INTERSECTION;
    }

    return TIG_OK;
}

// 0x52DF20
int tig_line_bounding_box(const TigLine* line, TigRect* rect)
{
    if (line->x2 <= line->x1) {
        rect->x = line->x2;
        rect->width = line->x1 - line->x2;
    } else {
        rect->x = line->x1;
        rect->width = line->x2 - line->x1;
    }

    if (line->y2 <= line->y1) {
        rect->y = line->y2;
        rect->height = line->y1 - line->y2;
    } else {
        rect->y = line->y1;
        rect->height = line->y2 - line->y1;
    }

    return TIG_OK;
}
