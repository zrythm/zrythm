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

#include "audio/engine.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/route_target_selector.h"
#include "gui/widgets/track_properties_expander.h"
#include "project.h"
#include "utils/string.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TrackPropertiesExpanderWidget,
               track_properties_expander_widget,
               TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

static void
on_midi_channels_changed (
  GtkComboBox * widget,
  TrackPropertiesExpanderWidget * self)
{
  const char * id =
    gtk_combo_box_get_active_id (widget);

  if (!id)
    return;

  Track * track = self->track;
  g_warn_if_fail (track);
  char * str;
  for (int i = 1; i <= 16; i++)
    {
      str =
        g_strdup_printf (
          "%d", i);
      if (string_is_equal (
            str, id, 1))
        {
          track->midi_ch = (uint8_t) i;
          g_free (str);
          break;
        }
      g_free (str);
    }
}

static void
setup_piano_roll_midi_channels_cb (
  TrackPropertiesExpanderWidget * self)
{
  GtkComboBoxText * cb = self->piano_roll_midi_ch;

  gtk_combo_box_text_remove_all (cb);

  char * id, * lbl;
  for (int i = 1; i <= 16; i++)
    {
      id = g_strdup_printf ("%d", i);
      lbl =
        g_strdup_printf (_("MIDI Channel %s"), id);

      gtk_combo_box_text_append (
        cb,
        id, lbl);

      g_free (id);
      g_free (lbl);
    }

  gtk_widget_set_visible (
    GTK_WIDGET (cb), 1);

  /* select the correct value */
  Track * track = self->track;
  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_STRING);
  if (track->midi_ch > 0)
    {
      id =
        g_strdup_printf ("%d", track->midi_ch);
      g_value_set_string (&a, id);
      g_free (id);
    }

  g_object_set_property (
    G_OBJECT (cb),
    "active-id",
    &a);

  gtk_widget_set_tooltip_text (
    GTK_WIDGET (cb),
    _("MIDI channel to send piano roll events to"));
}

/**
 * Refreshes each field.
 */
void
track_properties_expander_widget_refresh (
  TrackPropertiesExpanderWidget * self,
  Track *                         track)
{
  g_warn_if_fail (track);
  self->track = track;

  /* set midi channel for piano roll */
  if (track_has_piano_roll (self->track))
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->piano_roll_midi_ch), 1);
      gtk_widget_set_visible (
        GTK_WIDGET (self->piano_roll_midi_ch_lbl),
        1);
      setup_piano_roll_midi_channels_cb (
        self);
    }
  else
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->piano_roll_midi_ch), 0);
      gtk_widget_set_visible (
        GTK_WIDGET (self->piano_roll_midi_ch_lbl),
        0);
    }

  route_target_selector_widget_refresh (
    self->direct_out, track->channel);

  editable_label_widget_setup (
    self->name,
    track, track_get_name, track_set_name);
}

/**
 * Sets up the TrackPropertiesExpanderWidget.
 */
void
track_properties_expander_widget_setup (
  TrackPropertiesExpanderWidget * self,
  Track *                         track)
{
  g_warn_if_fail (track);
  self->track = track;

  char * str;
  GtkWidget * lbl;

#define CREATE_LABEL(x) \
  lbl = gtk_label_new (NULL); \
  gtk_label_set_xalign ( \
    GTK_LABEL (lbl), 0); \
  gtk_widget_set_margin_start ( \
    lbl, 2); \
  str = \
    g_strdup_printf ( \
      "%s%s%s", \
      "<b><small>", \
      x, "</small></b>"); \
  gtk_widget_set_visible (lbl, 1); \
  gtk_label_set_markup ( \
    GTK_LABEL (lbl), str); \
  g_free (str)

  /* add track name */
  self->name =
    editable_label_widget_new (
      NULL, NULL, NULL, 11);
  gtk_label_set_xalign (
    self->name->label, 0);
  gtk_widget_set_margin_start (
    GTK_WIDGET (self->name->label), 4);
  CREATE_LABEL (_("Track Name"));
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    lbl);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->name));

  /* add direct out */
  self->direct_out =
    route_target_selector_widget_new (
      track->channel);
  CREATE_LABEL (_("Direct Out"));
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    lbl);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->direct_out));

  /* add piano roll channel */
  self->piano_roll_midi_ch =
    GTK_COMBO_BOX_TEXT (
      gtk_combo_box_text_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->piano_roll_midi_ch), 1);
  g_signal_connect (
    self->piano_roll_midi_ch, "changed",
    G_CALLBACK (on_midi_channels_changed), self);
  CREATE_LABEL (_("Piano Roll"));
  self->piano_roll_midi_ch_lbl = lbl;
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    lbl);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->piano_roll_midi_ch));

#undef CREATE_LABEL

  /* set name and icon */
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    _("Track Properties"));
  expander_box_widget_set_icon_resource (
    Z_EXPANDER_BOX_WIDGET (self),
    ICON_TYPE_ZRYTHM,
    "instrument.svg");
  expander_box_widget_set_orientation (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_ORIENTATION_VERTICAL);

  track_properties_expander_widget_refresh (
    self, track);
}

static void
track_properties_expander_widget_class_init (
  TrackPropertiesExpanderWidgetClass * klass)
{
}

static void
track_properties_expander_widget_init (
  TrackPropertiesExpanderWidget * self)
{
}
