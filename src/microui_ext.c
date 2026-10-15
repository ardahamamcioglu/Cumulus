#include "microui_ext.h"

#include <string.h>

/* ------------------------------------------------------------------------- */
/* Progress bar                                                              */
/* ------------------------------------------------------------------------- */

void mu_ext_progress(mu_Context *ctx, mu_Real fraction)
{
    mu_Rect r = mu_layout_next(ctx);
    mu_Style *s = ctx->style;

    if (fraction < 0)
        fraction = 0;
    else if (fraction > 1)
        fraction = 1;

    /* track */
    mu_draw_rect(ctx, r, s->colors[MU_COLOR_SCROLLBASE]);

    /* fill */
    int w = (int)((r.w - 2) * fraction + 0.5f);
    if (w > 0)
    {
        mu_draw_rect(ctx, mu_rect(r.x + 1, r.y + 1, w, r.h - 2),
                     s->colors[MU_COLOR_SCROLLTHUMB]);
    }

    mu_draw_box(ctx, r, s->colors[MU_COLOR_BORDER]);
}

/* ------------------------------------------------------------------------- */
/* Radio button                                                              */
/* ------------------------------------------------------------------------- */

int mu_ext_radio(mu_Context *ctx, const char *label, int *value, int which)
{
    int res = 0;

    /* unique id per (value variable, option index) */
    mu_push_id(ctx, &which, sizeof(which));
    mu_Id id = mu_get_id(ctx, value, sizeof(value));
    mu_pop_id(ctx);

    mu_Rect r = mu_layout_next(ctx);
    mu_Rect box = mu_rect(r.x, r.y, r.h, r.h);
    mu_update_control(ctx, id, r, 0);

    if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id)
    {
        *value = which;
        res |= MU_RES_CHANGE;
    }

    int checked = (*value == which);
    mu_draw_control_frame(ctx, id, box, MU_COLOR_BASE, 0);
    if (checked)
    {
        mu_draw_icon(ctx, MU_ICON_CHECK, box, ctx->style->colors[MU_COLOR_TEXT]);
    }

    mu_draw_control_text(ctx, label,
                         mu_rect(r.x + box.w, r.y, r.w - box.w, r.h),
                         MU_COLOR_TEXT, 0);
    return res;
}

/* ------------------------------------------------------------------------- */
/* Dropdown                                                                  */
/* ------------------------------------------------------------------------- */

int mu_ext_dropdown(mu_Context *ctx, const char *name, int *selected,
                    const char *const *items, int count)
{
    int res = 0;
    if (count <= 0)
        return 0;

    mu_push_id(ctx, name, strlen(name));

    int idx = (*selected >= 0 && *selected < count) ? *selected : 0;
    const char *current = items[idx];

    if (mu_button(ctx, current))
    {
        mu_open_popup(ctx, name);
    }
    mu_Rect btn = ctx->last_rect;

    mu_Container *cnt = mu_get_container(ctx, name);
    if (cnt->open)
    {
        /* w/h are non-zero so mu_begin_window_ex() keeps our position */
        cnt->rect = mu_rect(btn.x, btn.y + btn.h, 1, 1);

        if (mu_begin_popup(ctx, name))
        {
            /* size the list to the widest item, at least as wide as the button */
            int w = btn.w;
            for (int i = 0; i < count; i++)
            {
                int tw = ctx->text_width(ctx->style->font, items[i], -1);
                if (tw > w)
                    w = tw;
            }
            w += 2 * ctx->style->padding;

            mu_layout_row(ctx, 1, (int[]){w}, 0);
            for (int i = 0; i < count; i++)
            {
                if (mu_button(ctx, items[i]))
                {
                    *selected = i;
                    res |= MU_RES_CHANGE;
                    cnt->open = 0;
                }
            }
            mu_end_popup(ctx);
        }
    }

    mu_pop_id(ctx);
    return res;
}

/* ------------------------------------------------------------------------- */
/* Color picker                                                              */
/* ------------------------------------------------------------------------- */

int mu_ext_color_picker(mu_Context *ctx, const char *name, mu_Color *color)
{
    int res = 0;

    mu_push_id(ctx, name, strlen(name));
    mu_Id id = mu_get_id(ctx, color, sizeof(color));
    mu_Rect r = mu_layout_next(ctx);
    mu_update_control(ctx, id, r, 0);

    if (ctx->mouse_pressed == MU_MOUSE_LEFT && ctx->focus == id)
    {
        mu_open_popup(ctx, name);
    }

    /* swatch: base frame + filled with current color */
    mu_draw_control_frame(ctx, id, r, MU_COLOR_BASE, 0);
    mu_draw_rect(ctx, mu_rect(r.x + 1, r.y + 1, r.w - 2, r.h - 2), *color);

    mu_Container *cnt = mu_get_container(ctx, name);
    if (cnt->open)
    {
        cnt->rect = mu_rect(r.x, r.y + r.h, 1, 1);

        if (mu_begin_popup(ctx, name))
        {
            enum { W = 160 };

            /* preview swatch */
            mu_layout_row(ctx, 1, (int[]){W}, ctx->text_height(ctx->style->font) + 8);
            mu_Rect prev = mu_layout_next(ctx);
            mu_draw_rect(ctx, prev, *color);
            mu_draw_box(ctx, prev, ctx->style->colors[MU_COLOR_BORDER]);

            /* R / G / B sliders */
            mu_Real rgb[3] = {color->r, color->g, color->b};
            static const char *labels[3] = {"R", "G", "B"};
            int changed = 0;
            for (int i = 0; i < 3; i++)
            {
                mu_layout_row(ctx, 2, (int[]){16, W - 16}, 0);
                mu_label(ctx, labels[i]);
                if (mu_slider_ex(ctx, &rgb[i], 0, 255, 1, "%.0f", MU_OPT_ALIGNCENTER) &
                    MU_RES_CHANGE)
                {
                    changed = 1;
                }
            }

            if (changed)
            {
                color->r = (unsigned char)rgb[0];
                color->g = (unsigned char)rgb[1];
                color->b = (unsigned char)rgb[2];
                res |= MU_RES_CHANGE;
            }

            mu_end_popup(ctx);
        }
    }

    mu_pop_id(ctx);
    return res;
}

/* ------------------------------------------------------------------------- */
/* Menu bar                                                                  */
/* ------------------------------------------------------------------------- */

#define MU_EXT_MENUBAR_NAME "##mu_ext_menubar"

int mu_ext_menu_bar(mu_Context *ctx, const mu_ext_Menu *menus, int count,
                    int *open)
{
    int result = -1;
    if (count <= 0)
    {
        *open = -1;
        return -1;
    }
    if (count > MU_MAX_WIDTHS)
        count = MU_MAX_WIDTHS;

    int widths[MU_MAX_WIDTHS];
    mu_Rect rects[MU_MAX_WIDTHS];
    for (int i = 0; i < count; i++)
        widths[i] = -1;

    mu_layout_row(ctx, count, widths, 0);
    for (int i = 0; i < count; i++)
    {
        if (mu_button(ctx, menus[i].label))
        {
            if (*open == i)
            {
                *open = -1;
            }
            else
            {
                *open = i;
                mu_open_popup(ctx, MU_EXT_MENUBAR_NAME);
            }
        }
        rects[i] = ctx->last_rect;
    }

    mu_Container *cnt = mu_get_container(ctx, MU_EXT_MENUBAR_NAME);
    if (*open >= 0 && *open < count)
    {
        int i = *open;
        cnt->rect = mu_rect(rects[i].x, rects[i].y + rects[i].h, 1, 1);

        if (mu_begin_popup(ctx, MU_EXT_MENUBAR_NAME))
        {
            /* width to the longest item, at least the menu's own width */
            int w = rects[i].w;
            for (int j = 0; j < menus[i].count; j++)
            {
                int tw = ctx->text_width(ctx->style->font, menus[i].items[j].label, -1);
                if (tw > w)
                    w = tw;
            }
            w += 2 * ctx->style->padding;

            mu_layout_row(ctx, 1, (int[]){w}, 0);
            for (int j = 0; j < menus[i].count; j++)
            {
                if (mu_button(ctx, menus[i].items[j].label))
                {
                    result = menus[i].items[j].action;
                    *open = -1;
                    cnt->open = 0;
                }
            }
            mu_end_popup(ctx);
        }

        /* popup may have been dismissed by an outside click */
        if (!cnt->open)
            *open = -1;
    }
    else
    {
        cnt->open = 0;
    }

    return result;
}

/* ------------------------------------------------------------------------- */
/* Tooltip                                                                   */
/* ------------------------------------------------------------------------- */

#define MU_EXT_TOOLTIP_NAME "##mu_ext_tooltip"

void mu_ext_tooltip(mu_Context *ctx, const char *text)
{
    mu_Rect r = ctx->last_rect;
    int show = mu_mouse_over(ctx, r);

    mu_Container *cnt = mu_get_container(ctx, MU_EXT_TOOLTIP_NAME);
    if (show)
    {
        cnt->rect = mu_rect(ctx->mouse_pos.x + 12, ctx->mouse_pos.y + 16, 1, 1);
        cnt->open = 1;
        mu_bring_to_front(ctx, cnt);

        if (mu_begin_popup(ctx, MU_EXT_TOOLTIP_NAME))
        {
            int tw = ctx->text_width(ctx->style->font, text, -1);
            mu_layout_row(ctx, 1, (int[]){tw}, 0);
            mu_label(ctx, text);
            mu_end_popup(ctx);
        }
    }
    else
    {
        cnt->open = 0;
    }
}
