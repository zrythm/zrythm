/*
 * inc/gui/widgets/quantize_mb_popover_popover.h - Snap and
 *   grid popover widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_QUANTIZE_MB_POPOVER_H__
#define __GUI_WIDGETS_QUANTIZE_MB_POPOVER_H__


#include <gtk/gtk.h>

#define QUANTIZE_MB_POPOVER_WIDGET_TYPE \
  (quantize_mb_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (QuantizeMbPopoverWidget,
                      quantize_mb_popover_widget,
                      Z,
                      QUANTIZE_MB_POPOVER_WIDGET,
                      GtkPopover)

typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct _QuantizeMbWidget QuantizeMbWidget;

typedef struct _QuantizeMbPopoverWidget
{
  GtkPopover              parent_instance;
  QuantizeMbWidget          * owner; ///< the owner
  GtkCheckButton          * use_grid;
  GtkBox *                note_length_box;
  DigitalMeterWidget      * dm_note_length; ///< digital meter for density
  GtkBox *                note_type_box;
  DigitalMeterWidget      * dm_note_type; ///< digital meter for note type
} QuantizeMbPopoverWidget;

/**
 * Creates the popover.
 */
QuantizeMbPopoverWidget *
quantize_mb_popover_widget_new (QuantizeMbWidget * owner);

#endif
