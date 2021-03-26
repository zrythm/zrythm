/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/active_hardware_mb.h"
#include "gui/widgets/active_hardware_popover.h"
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ActiveHardwareMbWidget,
  active_hardware_mb_widget,
  GTK_TYPE_MENU_BUTTON)

static void
on_clicked (
  GtkButton * button,
  ActiveHardwareMbWidget * self)
{
  gtk_widget_show_all (GTK_WIDGET (self->popover));
}

static void
set_label (ActiveHardwareMbWidget * self)
{
  gtk_label_set_text (
    self->label, _("Select..."));
}

/**
 * This is called when the popover closes.
 */
void
active_hardware_mb_widget_refresh (
  ActiveHardwareMbWidget * self)
{
  set_label (self);
  active_hardware_mb_widget_save_settings (self);
}

void
active_hardware_mb_widget_save_settings (
  ActiveHardwareMbWidget * self)
{
  GList *children, *iter;
  GtkToggleButton * chkbtn;
  char * controllers[40];
  int num_controllers = 0;

  children =
    gtk_container_get_children (
      GTK_CONTAINER (
        self->popover->controllers_box));
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      if (!GTK_IS_CHECK_BUTTON (iter->data))
        continue;

      chkbtn = GTK_TOGGLE_BUTTON (iter->data);
      if (gtk_toggle_button_get_active (chkbtn))
        controllers[num_controllers++] =
          g_strdup (gtk_button_get_label (
            GTK_BUTTON (chkbtn)));
    }
  controllers[num_controllers] = NULL;

  int res = 1;
  if (self->is_midi)
    {
      res =
        g_settings_set_strv (
          S_P_GENERAL_ENGINE,
          "midi-controllers",
          (const char * const*) controllers);
    }
  else
    {
      res =
        g_settings_set_strv (
          S_P_GENERAL_ENGINE,
          "audio-inputs",
          (const char * const*) controllers);
    }
  g_warn_if_fail (res == 1);

  g_list_free (children);
}

void
active_hardware_mb_widget_setup (
  ActiveHardwareMbWidget * self,
  bool                     is_midi)
{
  self->is_midi = is_midi;
  self->popover =
    active_hardware_popover_widget_new (self);
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self),
    GTK_WIDGET (self->popover));
}

static void
active_hardware_mb_widget_class_init (
  ActiveHardwareMbWidgetClass * klass)
{
}

static void
active_hardware_mb_widget_init (
  ActiveHardwareMbWidget * self)
{
  self->box =
    GTK_BOX (
      gtk_box_new (
        GTK_ORIENTATION_HORIZONTAL, 0));
  self->label =
    GTK_LABEL (gtk_label_new (_("Select...")));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->box),
    _("Click to enable inputs"));
  gtk_box_pack_start (
    self->box, GTK_WIDGET (self->label),
    F_EXPAND, F_NO_FILL, 1);
  gtk_container_add (
    GTK_CONTAINER (self), GTK_WIDGET (self->box));
  g_signal_connect (
    G_OBJECT (self), "clicked",
    G_CALLBACK (on_clicked), self);

  gtk_widget_show_all (GTK_WIDGET (self));
}
