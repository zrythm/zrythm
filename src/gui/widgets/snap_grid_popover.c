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

#include "audio/snap_grid.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/snap_grid_popover.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (SnapGridPopoverWidget,
               snap_grid_popover_widget,
               GTK_TYPE_POPOVER)

static void
on_closed (SnapGridPopoverWidget *self,
               gpointer    user_data)
{
  char * txt =
    snap_grid_stringize (self->owner->snap_grid->note_length,
                         self->owner->snap_grid->note_type);
  gtk_label_set_text (self->owner->label,
                      txt);
  g_free (txt);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
SnapGridPopoverWidget *
snap_grid_popover_widget_new (SnapGridWidget * owner)
{
  SnapGridPopoverWidget * self = g_object_new (SNAP_GRID_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;
  self->dm_note_length =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_NOTE_LENGTH,
      &owner->snap_grid->note_length,
      &owner->snap_grid->note_type,
      "note length");
  gtk_container_add (
    GTK_CONTAINER (self->note_length_box),
    GTK_WIDGET (self->dm_note_length));
  self->dm_note_type =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_NOTE_TYPE,
      &owner->snap_grid->note_length,
      &owner->snap_grid->note_type,
      "note type");
  gtk_container_add (
    GTK_CONTAINER (self->note_type_box),
    GTK_WIDGET (self->dm_note_type));

  return self;
}

static void
snap_grid_popover_widget_class_init (SnapGridPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "snap_grid_popover.ui");

  gtk_widget_class_bind_template_child (
    klass,
    SnapGridPopoverWidget,
    grid_adaptive);
  gtk_widget_class_bind_template_child (
    klass,
    SnapGridPopoverWidget,
    note_length_box);
  gtk_widget_class_bind_template_child (
    klass,
    SnapGridPopoverWidget,
    note_type_box);
  gtk_widget_class_bind_template_callback (
    klass,
    on_closed);
}

static void
snap_grid_popover_widget_init (SnapGridPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
