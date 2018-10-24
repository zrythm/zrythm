/*
 * gui/widgets/snap_grid_popover.c - Snap & grid_popover selection widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "project/snap_grid.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/snap_grid_popover.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (SnapGridPopoverWidget, snap_grid_popover_widget, GTK_TYPE_POPOVER)

static void
on_closed (SnapGridPopoverWidget *self,
               gpointer    user_data)
{
  char * txt = snap_grid_stringize (self->owner->snap_grid);
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
  self->dm_density = digital_meter_widget_new (DIGITAL_METER_TYPE_TIMESIG,
                                               owner->snap_grid);
  gtk_container_add (GTK_CONTAINER (self->density_box),
                     GTK_WIDGET (self->dm_density));


  return self;
}

static void
snap_grid_popover_widget_class_init (SnapGridPopoverWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/snap_grid_popover.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        SnapGridPopoverWidget,
                                        grid_adaptive);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        SnapGridPopoverWidget,
                                        density_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        SnapGridPopoverWidget,
                                        snap_grid);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        SnapGridPopoverWidget,
                                        snap_offset);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        SnapGridPopoverWidget,
                                        snap_events);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           on_closed);
}

static void
snap_grid_popover_widget_init (SnapGridPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


