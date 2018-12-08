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
#include "gui/widgets/arranger.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/piano_roll_page.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/piano_roll_labels.h"
#include "gui/widgets/piano_roll_notes.h"
#include "gui/widgets/ruler.h"

G_DEFINE_TYPE (PianoRollPageWidget,
               piano_roll_page_widget,
               GTK_TYPE_BOX)

/**
 * Sets up the MIDI editor.
 */
PianoRollPageWidget *
piano_roll_page_widget_new ()
{
  PianoRollPageWidget * self = g_object_new (PIANO_ROLL_PAGE_WIDGET_TYPE, NULL);
  MAIN_WINDOW->piano_roll_page = self;

  gtk_label_set_text (self->midi_name_label,
                      "Select a region...");

  GdkRGBA * color = calloc (1, sizeof (GdkRGBA));
  gdk_rgba_parse (color, "gray");
  self->midi_track_color = color_area_widget_new (color, 5, -1);
  gtk_box_pack_start (self->midi_track_color_box,
                      GTK_WIDGET (self->midi_track_color),
                      1,
                      1,
                      0);

  self->midi_ruler = ruler_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->midi_ruler_viewport),
                     GTK_WIDGET (self->midi_ruler));
  gtk_scrolled_window_set_hadjustment (self->midi_ruler_scroll,
                                       gtk_scrolled_window_get_hadjustment (
                                           GTK_SCROLLED_WINDOW (
                                               MAIN_WINDOW->timeline_scroll)));

  self->piano_roll_labels = piano_roll_labels_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->piano_roll_labels_viewport),
                     GTK_WIDGET (self->piano_roll_labels));

  self->piano_roll_notes = piano_roll_notes_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->piano_roll_notes_viewport),
                     GTK_WIDGET (self->piano_roll_notes));
  self->midi_arranger = midi_arranger_widget_new (
    &PROJECT->snap_grid_midi);
  gtk_container_add (GTK_CONTAINER (self->piano_roll_arranger_viewport),
                     GTK_WIDGET (self->midi_arranger));

  /* link scrolls */
  gtk_scrolled_window_set_vadjustment (
            self->piano_roll_labels_scroll,
            gtk_scrolled_window_get_vadjustment (
               GTK_SCROLLED_WINDOW (self->piano_roll_arranger_scroll)));
  gtk_scrolled_window_set_vadjustment (
            self->piano_roll_notes_scroll,
            gtk_scrolled_window_get_vadjustment (
               GTK_SCROLLED_WINDOW (self->piano_roll_arranger_scroll)));
  gtk_scrolled_window_set_hadjustment (self->piano_roll_arranger_scroll,
            gtk_scrolled_window_get_hadjustment (
               GTK_SCROLLED_WINDOW (MAIN_WINDOW->timeline_scroll)));

  return self;
}

static void
piano_roll_page_widget_class_init (PianoRollPageWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/piano_roll_page.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_track_color_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_bot_toolbar);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_name_label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_controls_above_notes_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_ruler_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_ruler_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_ruler_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_notes_labels_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        piano_roll_labels_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        piano_roll_labels_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_notes_draw_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        piano_roll_notes_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        piano_roll_notes_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        midi_arranger_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        piano_roll_arranger_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        PianoRollPageWidget,
                                        piano_roll_arranger_viewport);

}

static void
piano_roll_page_widget_init (PianoRollPageWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
