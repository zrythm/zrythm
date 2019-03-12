/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 */

#include "gui/backend/tool.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/top_dock_edge.h"
#include "project.h"
#include "utils/resources.h"

G_DEFINE_TYPE (ToolboxWidget,
               toolbox_widget,
               GTK_TYPE_BUTTON_BOX)

static int
on_motion (GtkWidget *      widget,
           GdkEventMotion * event,
           GtkToggleButton * self)
{
  if (event->type == GDK_ENTER_NOTIFY)
    {
      if (self == MW_TOOLBOX->select_mode)
        bot_bar_change_status (
          "Select Mode - Toggles between normal and "
          "stretch modes");
      else if (self == MW_TOOLBOX->edit_mode)
        bot_bar_change_status (
          "Edit Mode - Single click to create an "
          "object - Hold Ctrl while clicking and "
          "dragging to fill");
      else if (self == MW_TOOLBOX->erase_mode)
        bot_bar_change_status (
          "Eraser Mode");
      else if (self == MW_TOOLBOX->ramp_mode)
        bot_bar_change_status (
          "Ramp Mode - Useful when editing "
          "automation and velocities");
      else if (self == MW_TOOLBOX->audition_mode)
        bot_bar_change_status (
          "Audition Mode - Click and hold to listen "
          "to events");
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      bot_bar_change_status ("");
    }

  return FALSE;
}

static void
on_toggled (GtkToggleButton * tb,
            ToolboxWidget   * self)
{
  g_message ("toggling");
  if (tb == self->select_mode)
    {
      if (P_TOOL == TOOL_SELECT_NORMAL)
        P_TOOL = TOOL_SELECT_STRETCH;
      else
        P_TOOL = TOOL_SELECT_NORMAL;
    }
  else if (tb == self->edit_mode)
    P_TOOL = TOOL_EDIT;
  else if (tb == self->erase_mode)
    P_TOOL = TOOL_ERASER;
  else if (tb == self->ramp_mode)
    P_TOOL = TOOL_RAMP;
  else if (tb == self->audition_mode)
    P_TOOL = TOOL_AUDITION;

  toolbox_widget_refresh (MW_TOOLBOX);
}

/**
 * Sets the toolbox toggled states after deactivating
 * the callbacks.
 */
void
toolbox_widget_refresh (
  ToolboxWidget * self)
{
  g_message ("refreshing");
#define BLOCK_SIGNAL_HANDLER(lowercase) \
  g_signal_handler_block ( \
    self->lowercase##_mode, \
    self->lowercase##_handler_id)

  /* block signal handlers */
  BLOCK_SIGNAL_HANDLER (select);
  BLOCK_SIGNAL_HANDLER (edit);
  BLOCK_SIGNAL_HANDLER (erase);
  BLOCK_SIGNAL_HANDLER (ramp);
  BLOCK_SIGNAL_HANDLER (audition);

#undef BLOCK_SIGNAL_HANDLER

  /* set all inactive */
  gtk_toggle_button_set_active (
    self->select_mode, 0);
  gtk_toggle_button_set_active (
    self->edit_mode, 0);
  gtk_toggle_button_set_active (
    self->erase_mode, 0);
  gtk_toggle_button_set_active (
    self->ramp_mode, 0);
  gtk_toggle_button_set_active (
    self->audition_mode, 0);

  /* set select mode img */
  if (P_TOOL == TOOL_SELECT_STRETCH)
    gtk_image_set_from_icon_name (
      self->select_img,
      "selection-end-symbolic",
      GTK_ICON_SIZE_BUTTON);
  else
    gtk_image_set_from_icon_name (
      self->select_img,
      "edit-select",
      GTK_ICON_SIZE_BUTTON);

  /* set toggled states */
  if (P_TOOL == TOOL_SELECT_NORMAL ||
      P_TOOL == TOOL_SELECT_STRETCH)
    gtk_toggle_button_set_active (
      self->select_mode, 1);
  else if (P_TOOL == TOOL_EDIT)
    gtk_toggle_button_set_active (
      self->edit_mode, 1);
  else if (P_TOOL == TOOL_ERASER)
    gtk_toggle_button_set_active (
      self->erase_mode, 1);
  else if (P_TOOL == TOOL_RAMP)
    gtk_toggle_button_set_active (
      self->ramp_mode, 1);
  else if (P_TOOL == TOOL_AUDITION)
    gtk_toggle_button_set_active (
      self->audition_mode, 1);

#define UNBLOCK_SIGNAL_HANDLER(lowercase) \
  g_signal_handler_unblock ( \
    self->lowercase##_mode, \
    self->lowercase##_handler_id);

  /* unblock signal handlers */
  UNBLOCK_SIGNAL_HANDLER (select);
  UNBLOCK_SIGNAL_HANDLER (edit);
  UNBLOCK_SIGNAL_HANDLER (erase);
  UNBLOCK_SIGNAL_HANDLER (ramp);
  UNBLOCK_SIGNAL_HANDLER (audition);

#undef UNBLOCK_SIGNAL_HANDLER
}

static void
toolbox_widget_class_init (
  ToolboxWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "toolbox.ui");
  gtk_widget_class_set_css_name (
    klass, "toolbox");

  gtk_widget_class_bind_template_child (
    klass,
    ToolboxWidget,
    select_mode);
  gtk_widget_class_bind_template_child (
    klass,
    ToolboxWidget,
    edit_mode);
  gtk_widget_class_bind_template_child (
    klass,
    ToolboxWidget,
    erase_mode);
  gtk_widget_class_bind_template_child (
    klass,
    ToolboxWidget,
    ramp_mode);
  gtk_widget_class_bind_template_child (
    klass,
    ToolboxWidget,
    audition_mode);
  gtk_widget_class_bind_template_child (
    klass,
    ToolboxWidget,
    select_img);
}

static void
toolbox_widget_init (ToolboxWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define CONNECT_CLICK_HANDLER(lowercase) \
  self->lowercase##_handler_id = \
    g_signal_connect ( \
      G_OBJECT (self->lowercase##_mode), \
      "toggled", \
      G_CALLBACK (on_toggled), \
      self)

  /* connect click handlers */
  CONNECT_CLICK_HANDLER (select);
  CONNECT_CLICK_HANDLER (edit);
  CONNECT_CLICK_HANDLER (erase);
  CONNECT_CLICK_HANDLER (ramp);
  CONNECT_CLICK_HANDLER (audition);
#undef CONNECT_CLICK_HANDLER

#define CONNECT_NOTIFY_SIGNALS(lowercase) \
  g_signal_connect ( \
    G_OBJECT (self->lowercase##_mode), \
    "enter-notify-event", \
    G_CALLBACK (on_motion), self->lowercase##_mode); \
  g_signal_connect ( \
    G_OBJECT(self->lowercase##_mode), \
    "leave-notify-event", \
    G_CALLBACK (on_motion), self->lowercase##_mode);

  /* connect notify signals */
  CONNECT_NOTIFY_SIGNALS (select);
  CONNECT_NOTIFY_SIGNALS (edit);
  CONNECT_NOTIFY_SIGNALS (erase);
  CONNECT_NOTIFY_SIGNALS (ramp);
  CONNECT_NOTIFY_SIGNALS (audition);

#undef CONNECT_NOTIFY_SIGNALS
}
