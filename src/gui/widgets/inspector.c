/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/inspector_ap.h"
#include "gui/widgets/inspector_chord.h"
#include "gui/widgets/inspector_midi.h"
#include "gui/widgets/inspector_region.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (InspectorWidget, inspector_widget, GTK_TYPE_BOX)

InspectorWidget *
inspector_widget_new ()
{
  InspectorWidget * self = g_object_new (INSPECTOR_WIDGET_TYPE, NULL);


  self->track = inspector_track_widget_new ();
  self->region = inspector_region_widget_new ();
  self->ap = inspector_ap_widget_new ();
  self->midi = inspector_midi_widget_new ();
  self->chord = inspector_chord_widget_new ();
  gtk_box_pack_start (GTK_BOX (self->track_box),
                      GTK_WIDGET (self->track),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_FILL,
                      0);
  gtk_box_pack_start (GTK_BOX (self->region_box),
                      GTK_WIDGET (self->region),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_FILL,
                      0);
  gtk_box_pack_start (GTK_BOX (self->ap_box),
                      GTK_WIDGET (self->ap),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_FILL,
                      0);
  gtk_box_pack_start (GTK_BOX (self->midi_box),
                      GTK_WIDGET (self->midi),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_FILL,
                      0);
  gtk_box_pack_start (GTK_BOX (self->chord_box),
                      GTK_WIDGET (self->chord),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_FILL,
                      0);

  self->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (self->size_group,
                             GTK_WIDGET (self->region));
  /* TODO add the rest */

  GtkRequisition min_size, nat_size;
  gtk_widget_get_preferred_size (GTK_WIDGET (self->region),
                                 &min_size,
                                 &nat_size);
  gtk_widget_set_size_request (GTK_WIDGET (self),
                               min_size.width,
                               -1);

  gtk_widget_set_visible (GTK_WIDGET (self->region), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->ap), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->track), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->midi), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->no_item_label), 1);

  return self;
}

/**
 * Refreshes the inspector widget (shows current
 * selections.
 */
void
inspector_widget_refresh ()
{
  InspectorWidget * self = MW_INSPECTOR;

  gtk_widget_set_visible (
    GTK_WIDGET (self->region), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->ap), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->track), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->midi), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->chord), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->no_item_label), 0);

  if (TL_SELECTIONS->num_regions > 0)
    {
      inspector_region_widget_show_regions (
        self->region,
        (Region **) TL_SELECTIONS->regions,
        TL_SELECTIONS->num_regions);
      gtk_widget_set_visible (
        GTK_WIDGET (self->region), 1);
    }

  if (TL_SELECTIONS->num_automation_points > 0)
    {
      inspector_ap_widget_show_aps (
        self->ap,
        (AutomationPoint **) TL_SELECTIONS->automation_points,
        TL_SELECTIONS->num_automation_points);
      gtk_widget_set_visible (
        GTK_WIDGET (self->ap), 1);
    }
  if (MIDI_ARRANGER_SELECTIONS->num_midi_notes > 0)
    {
      inspector_midi_widget_show_midi (
        self->midi,
        (MidiNote **) MIDI_ARRANGER_SELECTIONS->midi_notes,
        MIDI_ARRANGER_SELECTIONS->num_midi_notes);
      gtk_widget_set_visible (
        GTK_WIDGET (self->midi), 1);

    }

  Track * selected_tracks[200];
  int     num_selected_tracks = 0;
  tracklist_get_selected_tracks (
    selected_tracks, &num_selected_tracks);
  if (num_selected_tracks > 0)
    {
      inspector_track_widget_show_tracks (
        self->track,
        (Track **) selected_tracks,
        num_selected_tracks);
      gtk_widget_set_visible (
        GTK_WIDGET (self->track), 1);
    }

  if (TL_SELECTIONS->num_chords > 0)
    {
      inspector_chord_widget_show_chords (
        self->chord,
        (Chord **) TL_SELECTIONS->chords,
        TL_SELECTIONS->num_chords);
      gtk_widget_set_visible (
        GTK_WIDGET (self->chord), 1);
    }

  /* if nothing is visible, show "no item selected" */
  if (!gtk_widget_get_visible (GTK_WIDGET (self->region)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->midi)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->track)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->chord)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->ap)))
    gtk_widget_set_visible (
      GTK_WIDGET (self->no_item_label), 1);
}

/**
 * Displays info about the regions/tracks/etc.
 *
 * If num_regions < 1, it hides the regions box, etc.
 */
void
inspector_widget_show_selections (
  InspectorWidgetChildType type,
  void **                  selections,
  int                      num_selections)
{

  InspectorWidget * self = MW_INSPECTOR;
  if (type == INSPECTOR_CHILD_REGION)
    gtk_widget_set_visible (GTK_WIDGET (self->region), 0);
  else if (type == INSPECTOR_CHILD_AP)
    gtk_widget_set_visible (GTK_WIDGET (self->ap), 0);
  else if (type == INSPECTOR_CHILD_TRACK)
    gtk_widget_set_visible (GTK_WIDGET (self->track), 0);
  else if (type == INSPECTOR_CHILD_MIDI)
    gtk_widget_set_visible (GTK_WIDGET (self->midi), 0);
  else if (type == INSPECTOR_CHILD_CHORD)
    gtk_widget_set_visible (GTK_WIDGET (self->chord), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->no_item_label), 0);
  if (num_selections > 0)
    {
      if (type == INSPECTOR_CHILD_REGION)
        {
          inspector_region_widget_show_regions (
            self->region,
            (Region **) selections,
            num_selections);
          gtk_widget_set_visible (
            GTK_WIDGET (self->region), 1);
        }
      else if (type == INSPECTOR_CHILD_AP)
        {
          inspector_ap_widget_show_aps (
            self->ap,
            (AutomationPoint **) selections,
            num_selections);
          gtk_widget_set_visible (
            GTK_WIDGET (self->ap), 1);
        }
      else if (type == INSPECTOR_CHILD_MIDI)
        {
          inspector_midi_widget_show_midi (
            self->midi,
            (MidiNote **) selections,
            num_selections);
          gtk_widget_set_visible (
            GTK_WIDGET (self->midi), 1);

        }
      else if (type == INSPECTOR_CHILD_TRACK)
        {
          inspector_track_widget_show_tracks (
            self->track,
            (Track **) selections,
            num_selections);
          gtk_widget_set_visible (
            GTK_WIDGET (self->track), 1);
        }
      else if (type == INSPECTOR_CHILD_CHORD)
        {
          inspector_chord_widget_show_chords (
            self->chord,
            (Chord **) selections,
            num_selections);
          gtk_widget_set_visible (
            GTK_WIDGET (self->chord), 1);
        }
    }

  /* if nothing is visible, show "no item selected" */
  if (!gtk_widget_get_visible (GTK_WIDGET (self->region)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->midi)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->track)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->chord)) &&
      !gtk_widget_get_visible (GTK_WIDGET (self->ap)))
    gtk_widget_set_visible (
      GTK_WIDGET (self->no_item_label), 1);
}

static void
inspector_widget_class_init (
  InspectorWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "inspector.ui");

  gtk_widget_class_set_css_name (klass,
                                 "inspector");

  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    top_box);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    track_box);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    region_box);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    ap_box);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    bot_box);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    midi_box);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    chord_box);
  gtk_widget_class_bind_template_child (
    klass,
    InspectorWidget,
    no_item_label);
}

static void
inspector_widget_init (InspectorWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
