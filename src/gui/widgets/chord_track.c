/*
 * gui/widgets/chord_track.c - Track
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

/** \file
 */

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/chord_track.h"
#include "audio/region.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/chord_track.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (ChordTrackWidget, chord_track_widget, GTK_TYPE_PANED)

/**
 * Creates a new track widget using the given track.
 *
 * 1 track has 1 track widget.
 * The track widget must always have at least 1 automation track in the automation
 * paned.
 */
ChordTrackWidget *
chord_track_widget_new (ChordTrack * track,
                        TrackWidget * parent)
{
  ChordTrackWidget * self = g_object_new (
                            CHORD_TRACK_WIDGET_TYPE,
                            NULL);
  self->parent = parent;

  chord_track_widget_update_all (self);

  gtk_container_add (
    GTK_CONTAINER (self->mute),
    gtk_image_new_from_resource (
     "/online/alextee/zrythm/mute.svg"));

  gtk_image_set_from_resource (self->icon,
                               "/online/alextee/zrythm/chord.svg");

  return self;
}

static void
chord_track_widget_init (ChordTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
chord_track_widget_class_init (ChordTrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/chord_track.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    ChordTrackWidget,
    track_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    ChordTrackWidget,
    track_grid);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    ChordTrackWidget,
    track_name);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    ChordTrackWidget,
    record);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    ChordTrackWidget,
    solo);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    ChordTrackWidget,
    mute);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    ChordTrackWidget,
    icon);
}

void
chord_track_widget_update_all (ChordTrackWidget * self)
{
  gtk_label_set_text (self->track_name,
                      "Chord Track");
}

/**
 * Makes sure the track widget and its elements have the visibility they should.
 */
void
chord_track_widget_show (ChordTrackWidget * self)
{
  g_message ("showing Chord track widget");
  gtk_widget_show (GTK_WIDGET (self));
  gtk_widget_show_all (GTK_WIDGET (self->track_box));
}
