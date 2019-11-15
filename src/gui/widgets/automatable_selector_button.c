/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/engine.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/widgets/automatable_selector_button.h"
#include "gui/widgets/automatable_selector_popover.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (AutomatableSelectorButtonWidget,
               automatable_selector_button_widget,
               GTK_TYPE_MENU_BUTTON)

static void
on_clicked (GtkButton * button,
            AutomatableSelectorButtonWidget * self)
{
  gtk_widget_set_visible (
    GTK_WIDGET (self->popover), 1);
}

static void
set_label (AutomatableSelectorButtonWidget * self)
{
  if (self->owner->at->automatable)
    {
      gtk_label_set_text (
        self->label,
        self->owner->at->automatable->label);
    }
}

/**
 * Sets the Automatable for this automation track
 * lane.
 *
 * If different from the current one, it will hide
 * the current AutomationTrack and show the
 * one corresponding to this Automatable.
 */
void
automatable_selector_button_set_automatable (
  AutomatableSelectorButtonWidget * self,
  Automatable *                     a)
{
  AutomationTrack * at = self->owner->at;
  if (a && at->automatable != a)
    {
      at->visible = 0;
      /* TODO swap indices */
      at =
        automation_tracklist_get_at_from_automatable (
          track_get_automation_tracklist (at->track),
          a);
      at->created = 1;
      at->visible = 1;
      EVENTS_PUSH (ET_AUTOMATION_TRACK_ADDED,
                   at);
    }
}

void
automatable_selector_button_widget_refresh (
  AutomatableSelectorButtonWidget * self)
{
  set_label (self);
}

void
automatable_selector_button_widget_setup (
  AutomatableSelectorButtonWidget * self,
  AutomationTrackWidget *            owner)
{
  self->owner = owner;
  self->popover =
    automatable_selector_popover_widget_new (self);
  gtk_menu_button_set_popover (
    GTK_MENU_BUTTON (self),
    GTK_WIDGET (self->popover));

  set_label (self);
}

static void
automatable_selector_button_widget_class_init (
  AutomatableSelectorButtonWidgetClass * klass)
{
}

static void
automatable_selector_button_widget_init (
  AutomatableSelectorButtonWidget * self)
{
  self->box =
    GTK_BOX (gtk_box_new (
              GTK_ORIENTATION_HORIZONTAL, 0));
  self->img = GTK_IMAGE (
    gtk_image_new_from_icon_name (
      "z-node-type-cusp", GTK_ICON_SIZE_BUTTON));
  self->label =
    GTK_LABEL (gtk_label_new ("Automation"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->box),
    "Select control to automate");
  gtk_box_pack_start (self->box,
                      GTK_WIDGET (self->img),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      1);
  gtk_box_pack_start (self->box,
                    GTK_WIDGET (self->label),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    1);
  gtk_container_add (GTK_CONTAINER (self),
                     GTK_WIDGET (self->box));
  g_signal_connect (G_OBJECT (self),
                    "clicked",
                    G_CALLBACK (on_clicked),
                    self);

  gtk_widget_show_all (GTK_WIDGET (self));
}


