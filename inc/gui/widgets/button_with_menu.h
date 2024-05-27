/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Button with an arrow on the right side for a menu.
 */

#ifndef __GUI_WIDGETS_BUTTON_WITH_MENU_H__
#define __GUI_WIDGETS_BUTTON_WITH_MENU_H__

#include "gtk_wrapper.h"

#define BUTTON_WITH_MENU_WIDGET_TYPE (button_with_menu_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ButtonWithMenuWidget,
  button_with_menu_widget,
  Z,
  BUTTON_WITH_MENU_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Button with a separator and an arrow for a menu.
 */
typedef struct _ButtonWithMenuWidget
{
  GtkBox parent_instance;

  GtkBox * button_box;

  /** True for downward arrow, false for upward
   * arrow. */
  bool downard_arrow;

  GtkMenuButton * menu_btn;

  /** The menu to show when arrow is clicked. */
  // GtkMenu *        menu;
} ButtonWithMenuWidget;

void
button_with_menu_widget_set_menu_model (
  ButtonWithMenuWidget * self,
  GMenuModel *           gmenu_model);

/**
 * Set a custom popover instead of a menu model.
 */
void
button_with_menu_widget_set_popover (
  ButtonWithMenuWidget * self,
  GtkPopover *           popover);

/**
 * This must only be called once to set up the
 * widget.
 *
 * @param btn The main button.
 * @param menu Optional GtkMenu to set for the
 *   arrow button.
 * @param menu Optional GMenuModel to set for the
 *   arrow button.
 */
void
button_with_menu_widget_setup (
  ButtonWithMenuWidget * self,
  GtkButton *            btn,
  GMenuModel *           gmenu_model,
  bool                   downward_arrow,
  int                    height,
  const char *           btn_tooltip_text,
  const char *           menu_tooltip_text);

ButtonWithMenuWidget *
button_with_menu_widget_new (void);

static inline GtkMenuButton *
button_with_menu_widget_get_menu_button (ButtonWithMenuWidget * self)
{
  return self->menu_btn;
}

/**
 * @}
 */

#endif
