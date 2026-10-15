/*
 * microui_ext — extra widgets on top of microui's public API.
 *
 * These are implemented entirely with microui's documented functions (layout,
 * popups, and the mu_draw_* primitives), so the vendored microui library is
 * left unmodified. They follow microui's immediate-mode conventions: call them
 * every frame inside a window/panel, between mu_begin_window/panel and
 * mu_end_window/panel.
 *
 * Return conventions match microui: functions return MU_RES_CHANGE (non-zero)
 * when their value changed, otherwise 0. The menu bar returns the selected
 * item's action id, or -1.
 */

#ifndef CUMULUS_MICROUI_EXT_H
#define CUMULUS_MICROUI_EXT_H

#include "microui.h"

/* A horizontal bar of progress in [0, 1]. Consumes one layout slot. */
void mu_ext_progress(mu_Context *ctx, mu_Real fraction);

/* One radio option. `value` points at the shared selection; `which` is this
 * option's value. Selecting this option sets *value = which. Returns
 * MU_RES_CHANGE when it was just selected. */
int mu_ext_radio(mu_Context *ctx, const char *label, int *value, int which);

/* A dropdown/combobox. `name` must be unique per widget (used as the popup
 * identity). Shows items[*selected] as a button; clicking opens a popup list.
 * Returns MU_RES_CHANGE when the selection changed. */
int mu_ext_dropdown(mu_Context *ctx, const char *name, int *selected,
                    const char *const *items, int count);

/* A color swatch button that opens a popup with R/G/B sliders + a preview.
 * `name` must be unique. Returns MU_RES_CHANGE when the color changed. */
int mu_ext_color_picker(mu_Context *ctx, const char *name, mu_Color *color);

typedef struct {
    const char *label;
    int action;
} mu_ext_MenuItem;

typedef struct {
    const char *label;
    const mu_ext_MenuItem *items;
    int count;
} mu_ext_Menu;

/* A top-level menu bar. `open` is caller-owned state (index of the open menu,
 * or -1). Returns the clicked item's action id, or -1. */
int mu_ext_menu_bar(mu_Context *ctx, const mu_ext_Menu *menus, int count,
                    int *open);

/* Show `text` near the mouse while the previously-drawn widget is hovered.
 * Call it immediately after the widget it should describe. */
void mu_ext_tooltip(mu_Context *ctx, const char *text);

#endif /* CUMULUS_MICROUI_EXT_H */
