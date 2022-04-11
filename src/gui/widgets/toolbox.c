/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 */

#include "gui/backend/tool.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/header.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/toolbox.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  ToolboxWidget,
  toolbox_widget,
  GTK_TYPE_BOX)

static void
on_toggled (
  GtkToggleButton * tb,
  ToolboxWidget *   self)
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
  else if (tb == self->cut_mode)
    P_TOOL = TOOL_CUT;
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
toolbox_widget_refresh (ToolboxWidget * self)
{
#define BLOCK_SIGNAL_HANDLER(lowercase) \
  g_signal_handler_block ( \
    self->lowercase##_mode, \
    self->lowercase##_handler_id)

  /* block signal handlers */
  BLOCK_SIGNAL_HANDLER (select);
  BLOCK_SIGNAL_HANDLER (edit);
  BLOCK_SIGNAL_HANDLER (cut);
  BLOCK_SIGNAL_HANDLER (erase);
  BLOCK_SIGNAL_HANDLER (ramp);
  BLOCK_SIGNAL_HANDLER (audition);

#undef BLOCK_SIGNAL_HANDLER

  /* set all inactive */
  gtk_toggle_button_set_active (
    self->select_mode, 0);
  gtk_toggle_button_set_active (self->edit_mode, 0);
  gtk_toggle_button_set_active (self->cut_mode, 0);
  gtk_toggle_button_set_active (
    self->erase_mode, 0);
  gtk_toggle_button_set_active (self->ramp_mode, 0);
  gtk_toggle_button_set_active (
    self->audition_mode, 0);

  /* set select mode img */
  gtk_image_set_from_icon_name (
    self->select_img,
    P_TOOL == TOOL_SELECT_STRETCH
      ? "selection-end-symbolic"
      : "edit-select");

  /* set toggled states */
  switch (P_TOOL)
    {
    case TOOL_SELECT_NORMAL:
    case TOOL_SELECT_STRETCH:
      gtk_toggle_button_set_active (
        self->select_mode, true);
      break;
    case TOOL_EDIT:
      gtk_toggle_button_set_active (
        self->edit_mode, 1);
      break;
    case TOOL_CUT:
      gtk_toggle_button_set_active (
        self->cut_mode, 1);
      break;
    case TOOL_ERASER:
      gtk_toggle_button_set_active (
        self->erase_mode, 1);
      break;
    case TOOL_RAMP:
      gtk_toggle_button_set_active (
        self->ramp_mode, 1);
      break;
    case TOOL_AUDITION:
      gtk_toggle_button_set_active (
        self->audition_mode, 1);
      break;
    }

#define UNBLOCK_SIGNAL_HANDLER(lowercase) \
  g_signal_handler_unblock ( \
    self->lowercase##_mode, \
    self->lowercase##_handler_id);

  /* unblock signal handlers */
  UNBLOCK_SIGNAL_HANDLER (select);
  UNBLOCK_SIGNAL_HANDLER (edit);
  UNBLOCK_SIGNAL_HANDLER (cut);
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
  gtk_widget_class_set_css_name (klass, "toolbox");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ToolboxWidget, x)

  BIND_CHILD (select_mode);
  BIND_CHILD (edit_mode);
  BIND_CHILD (cut_mode);
  BIND_CHILD (erase_mode);
  BIND_CHILD (ramp_mode);
  BIND_CHILD (audition_mode);
  BIND_CHILD (select_img);

#undef BIND_CHILD
}

static void
connect_notify_signals (
  ToolboxWidget * self,
  GtkWidget *     child)
{
#if 0
  GtkEventControllerMotion * motion_controller;
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller),  "enter",
    G_CALLBACK (on_enter), child);
  g_signal_connect (
    G_OBJECT (motion_controller),  "leave",
    G_CALLBACK (on_leave), child);
  gtk_widget_add_controller (
    child,
    GTK_EVENT_CONTROLLER (motion_controller));
#endif
}

static void
toolbox_widget_init (ToolboxWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#define CONNECT_CLICK_HANDLER(lowercase) \
  self->lowercase##_handler_id = g_signal_connect ( \
    G_OBJECT (self->lowercase##_mode), "toggled", \
    G_CALLBACK (on_toggled), self)

  /* connect click handlers */
  CONNECT_CLICK_HANDLER (select);
  CONNECT_CLICK_HANDLER (edit);
  CONNECT_CLICK_HANDLER (cut);
  CONNECT_CLICK_HANDLER (erase);
  CONNECT_CLICK_HANDLER (ramp);
  CONNECT_CLICK_HANDLER (audition);
#undef CONNECT_CLICK_HANDLER

#define CONNECT_NOTIFY_SIGNALS(lowercase) \
  connect_notify_signals ( \
    self, GTK_WIDGET (self->lowercase##_mode))

  /* connect notify signals */
  CONNECT_NOTIFY_SIGNALS (select);
  CONNECT_NOTIFY_SIGNALS (edit);
  CONNECT_NOTIFY_SIGNALS (cut);
  CONNECT_NOTIFY_SIGNALS (erase);
  CONNECT_NOTIFY_SIGNALS (ramp);
  CONNECT_NOTIFY_SIGNALS (audition);

#undef CONNECT_NOTIFY_SIGNALS

  const char * tooltip_text;
#define SET_TOOLTIP(x, action) \
  tooltip_text = gtk_widget_get_tooltip_text ( \
    GTK_WIDGET (self->x##_mode)); \
  z_gtk_widget_set_tooltip_for_action ( \
    GTK_WIDGET (self->x##_mode), action, \
    tooltip_text);

  SET_TOOLTIP (select, "app.select-mode");
  SET_TOOLTIP (edit, "app.edit-mode");
  SET_TOOLTIP (cut, "app.cut-mode");
  SET_TOOLTIP (erase, "app.eraser-mode");
  SET_TOOLTIP (ramp, "app.ramp-mode");
  SET_TOOLTIP (audition, "app.audition-mode");

#undef SET_TOOLTIP
}
