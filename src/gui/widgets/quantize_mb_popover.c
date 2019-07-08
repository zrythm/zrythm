/*
 * gui/widgets/quantize_mb_popover.c - Snap & grid_popover selection widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/quantize.h"
#include "gui/widgets/quantize_mb.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/quantize_mb_popover.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (QuantizeMbPopoverWidget,
               quantize_mb_popover_widget,
               GTK_TYPE_POPOVER)

static void
on_closed (QuantizeMbPopoverWidget *self,
               gpointer    user_data)
{
  char * txt =
    snap_grid_stringize (self->owner->quantize->note_length,
                         self->owner->quantize->note_type);
  gtk_label_set_text (self->owner->label,
                      txt);
  g_free (txt);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
QuantizeMbPopoverWidget *
quantize_mb_popover_widget_new (QuantizeMbWidget * owner)
{
  QuantizeMbPopoverWidget * self = g_object_new (QUANTIZE_MB_POPOVER_WIDGET_TYPE, NULL);

  self->owner = owner;
  self->dm_note_length =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_NOTE_LENGTH,
      &owner->quantize->note_length,
      &owner->quantize->note_type,
      "note length");
  gtk_container_add (GTK_CONTAINER (self->note_length_box),
                     GTK_WIDGET (self->dm_note_length));
  self->dm_note_type =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_NOTE_TYPE,
      &owner->quantize->note_length,
      &owner->quantize->note_type,
      "note type");
  gtk_container_add (GTK_CONTAINER (self->note_type_box),
                     GTK_WIDGET (self->dm_note_type));

  return self;
}

static void
quantize_mb_popover_widget_class_init (QuantizeMbPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "quantize_mb_popover.ui");

  gtk_widget_class_bind_template_child (
    klass,
    QuantizeMbPopoverWidget,
    use_grid);
  gtk_widget_class_bind_template_child (
    klass,
    QuantizeMbPopoverWidget,
    note_length_box);
  gtk_widget_class_bind_template_child (
    klass,
    QuantizeMbPopoverWidget,
    note_type_box);
  gtk_widget_class_bind_template_callback (
    klass,
    on_closed);
}

static void
quantize_mb_popover_widget_init (QuantizeMbPopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
