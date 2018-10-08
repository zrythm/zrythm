/*
 * inc/gui/widgets/editor_notebook.c - Editor notebook (bot of arranger)
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_editor.h"
#include "gui/widgets/piano_roll_arranger.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/piano_roll_notes.h"
#include "gui/widgets/ruler.h"

/**
 * Sets up the MIDI editor for the given region.
 */
void
midi_editor_set_region (Region * region)
{
  gtk_notebook_set_current_page (MAIN_WINDOW->editor_bot, 0);

  char * label = g_strdup_printf ("%s - %s",
                                       region->track->channel->name,
                                       region->name);
  gtk_label_set_text (MAIN_WINDOW->midi_name_label,
                      label);
  g_free (label);

  color_area_widget_set_color (MAIN_WINDOW->midi_track_color,
                               &region->track->channel->color);
}

/**
 * Sets up the MIDI editor.
 */
void
midi_editor_setup ()
{
  gtk_notebook_set_current_page (MAIN_WINDOW->editor_bot, 0);

  gtk_label_set_text (MAIN_WINDOW->midi_name_label,
                      "Select a region...");

  GdkRGBA * color = calloc (1, sizeof (GdkRGBA));
  gdk_rgba_parse (color, "gray");
  MAIN_WINDOW->midi_track_color = color_area_widget_new (color, 5, -1);
  gtk_box_pack_start (MAIN_WINDOW->midi_track_color_box,
                      GTK_WIDGET (MAIN_WINDOW->midi_track_color),
                      1,
                      1,
                      0);

  MAIN_WINDOW->midi_ruler = ruler_widget_new ();
  gtk_container_add (GTK_CONTAINER (MAIN_WINDOW->midi_ruler_viewport),
                     GTK_WIDGET (MAIN_WINDOW->midi_ruler));
  gtk_scrolled_window_set_hadjustment (MAIN_WINDOW->midi_ruler_scroll,
                                       gtk_scrolled_window_get_hadjustment (
                                           GTK_SCROLLED_WINDOW (
                                               MAIN_WINDOW->timeline_scroll)));

  MAIN_WINDOW->piano_roll_labels = piano_roll_labels_widget_new ();
  gtk_container_add (GTK_CONTAINER (MAIN_WINDOW->piano_roll_labels_viewport),
                     GTK_WIDGET (MAIN_WINDOW->piano_roll_labels));

  MAIN_WINDOW->piano_roll_notes = piano_roll_notes_widget_new ();
  gtk_container_add (GTK_CONTAINER (MAIN_WINDOW->piano_roll_notes_viewport),
                     GTK_WIDGET (MAIN_WINDOW->piano_roll_notes));
  MAIN_WINDOW->piano_roll_arranger = piano_roll_arranger_widget_new ();
  gtk_container_add (GTK_CONTAINER (MAIN_WINDOW->piano_roll_arranger_viewport),
                     GTK_WIDGET (MAIN_WINDOW->piano_roll_arranger));

  /* link scrolls */
  gtk_scrolled_window_set_vadjustment (
            MAIN_WINDOW->piano_roll_labels_scroll,
            gtk_scrolled_window_get_vadjustment (
               GTK_SCROLLED_WINDOW (MAIN_WINDOW->piano_roll_arranger_scroll)));
  gtk_scrolled_window_set_vadjustment (
            MAIN_WINDOW->piano_roll_notes_scroll,
            gtk_scrolled_window_get_vadjustment (
               GTK_SCROLLED_WINDOW (MAIN_WINDOW->piano_roll_arranger_scroll)));
  gtk_scrolled_window_set_hadjustment (MAIN_WINDOW->piano_roll_arranger_scroll,
            gtk_scrolled_window_get_hadjustment (
               GTK_SCROLLED_WINDOW (MAIN_WINDOW->timeline_scroll)));


  gtk_widget_show_all (GTK_WIDGET (MAIN_WINDOW->editor_bot));
}
