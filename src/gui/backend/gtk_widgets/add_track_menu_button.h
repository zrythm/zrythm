// SPDX-FileCopyrightText: © 2022 Robert Panovics <robert dot panovics at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Button with a popover menu to add new tracks
 */

#ifndef __GUI_WIDGETS_ADD_TRACK_MENU_BUTTON_H__
#define __GUI_WIDGETS_ADD_TRACK_MENU_BUTTON_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define ADD_TRACK_MENU_BUTTON_WIDGET_TYPE \
  (add_track_menu_button_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AddTrackMenuButtonWidget,
  add_track_menu_button_widget,
  Z,
  ADD_TRACK_MENU_BUTTON_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Button with a popover menu.
 */
typedef struct _AddTrackMenuButtonWidget
{
  GtkWidget parent_instance;

  GtkMenuButton * menu_btn;

} AddTrackMenuButtonWidget;

AddTrackMenuButtonWidget *
add_track_menu_button_widget_new (void);

/**
 * @}
 */

#endif
