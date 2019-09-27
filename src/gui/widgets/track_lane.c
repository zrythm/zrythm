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

#include "audio/track_lane.h"
#include "gui/widgets/track_lane.h"
#include "project.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TrackLaneWidget,
               track_lane_widget,
               GTK_TYPE_GRID)

static void
on_midi_channels_changed (
  GtkComboBox * widget,
  TrackLaneWidget * self)
{
  const char * id =
    gtk_combo_box_get_active_id (widget);

  if (!id)
    return;

  TrackLane * lane = self->lane;
  if (string_is_equal (id, "inherit", 1))
    {
      lane->midi_ch = 0;
    }
  else
    {
      char * str;
      for (int i = 1; i <= 16; i++)
        {
          str =
            g_strdup_printf (
              "%d", i);
          if (string_is_equal (
                str, id, 1))
            {
              lane->midi_ch = (uint8_t) i;
              g_free (str);
              break;
            }
          g_free (str);
        }
    }
}

static int
row_separator_func (
  GtkTreeModel *model,
  GtkTreeIter *iter,
  TrackLaneWidget * self)
{
  GValue a = G_VALUE_INIT;
  /*g_value_init (&a, G_TYPE_STRING);*/
  gtk_tree_model_get_value (model, iter, 0, &a);
  const char * val = g_value_get_string (&a);
  if (val && string_is_equal (val, "separator", 1))
    return 1;
  else
    return 0;
}

static void
setup_midi_channels_cb (
  TrackLaneWidget * self)
{
  GtkComboBoxText * cb = self->midi_ch_cb;

  gtk_combo_box_text_remove_all (
    cb);

  gtk_combo_box_text_append (
    cb,
    "inherit", _("Inherit"));
  gtk_combo_box_text_append (
    cb,
    "separator", "separator");

  char * id, * lbl;
  for (int i = 1; i <= 16; i++)
    {
      id = g_strdup_printf ("%d", i);
      lbl = g_strdup_printf (_("Channel %s"), id);

      gtk_combo_box_text_append (
        cb,
        id, lbl);

      g_free (id);
      g_free (lbl);
    }

  gtk_widget_set_visible (
    GTK_WIDGET (cb), 1);

  /* select the correct value */
  TrackLane * lane = self->lane;
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_STRING);
  if (lane->midi_ch > 0)
    {
      g_message ("setting %d", lane->midi_ch);
      id = g_strdup_printf ("%d", lane->midi_ch);
      g_value_set_string (&a, id);
      g_free (id);
    }
  else if (lane->midi_ch == 0)
    {
      g_value_set_string (&a, "inherit");
    }
  else
    g_warn_if_reached ();

  g_object_set_property (
    G_OBJECT (cb),
    "active-id",
    &a);
}

/**
 * Updates changes in the backend to the ui.
 */
void
track_lane_widget_refresh (
  TrackLaneWidget * self)
{
  gtk_label_set_text (
    self->label,
    self->lane->name);

  /* setup midi channels combo box */
  if (track_has_piano_roll (self->lane->track))
    setup_midi_channels_cb (self);
  g_message ("refreshing");
}

TrackLaneWidget *
track_lane_widget_new (
  TrackLane * lane)
{
  TrackLaneWidget * self =
    g_object_new (TRACK_LANE_WIDGET_TYPE, NULL);

  self->lane = lane;

  /* setup midi channel box */
  if (track_has_piano_roll (lane->track))
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->midi_ch_box), 1);
      gtk_combo_box_set_row_separator_func (
        GTK_COMBO_BOX (self->midi_ch_cb),
        (GtkTreeViewRowSeparatorFunc)
          row_separator_func,
        self, NULL);
      g_signal_connect (
        self->midi_ch_cb, "changed",
        G_CALLBACK (on_midi_channels_changed), self);
    }

  track_lane_widget_refresh (
    self);

  g_object_ref (self);

  return self;
}

static void
track_lane_widget_class_init (
  TrackLaneWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "track_lane.ui");

  gtk_widget_class_set_css_name (
    klass, "track-lane");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    TrackLaneWidget, \
    x)

  BIND_CHILD (label);
  BIND_CHILD (mute);
  BIND_CHILD (solo);
  BIND_CHILD (midi_ch_box);
  BIND_CHILD (midi_ch_cb);
}

static void
track_lane_widget_init (TrackLaneWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_visible (GTK_WIDGET (self), 1);
  gtk_widget_set_vexpand (GTK_WIDGET (self), 1);
  gtk_widget_set_size_request (
    GTK_WIDGET (self), -1, 42);
}
