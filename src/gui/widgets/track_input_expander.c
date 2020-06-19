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
#include "audio/ext_port.h"
#include "audio/track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/track_input_expander.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/string.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TrackInputExpanderWidget,
               track_input_expander_widget,
               TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

static void
on_midi_channels_changed (
  GtkComboBox * widget,
  TrackInputExpanderWidget * self)
{
  const char * id =
    gtk_combo_box_get_active_id (widget);

  if (!id)
    return;

  Channel * ch = self->track->channel;
  if (string_is_equal (id, "all", 1))
    {
      if (ch->all_midi_channels)
        return;
      ch->all_midi_channels = 1;
    }
  else if (string_is_equal (id, "none", 1))
    {
      ch->all_midi_channels = 0;
      for (int i = 0; i < 16; i++)
        {
          ch->midi_channels[i] = 0;
        }
    }
  else
    {
      ch->all_midi_channels = 0;

      /* clear */
      for (int i = 0; i < 16; i++)
        {
          ch->midi_channels[i] = 0;
        }

      char * str;
      for (int i = 0; i < 16; i++)
        {
          str =
            g_strdup_printf (
              "%d", i + 1);
          if (string_is_equal (
                str, id, 1))
            {
              ch->midi_channels[i] = 1;
              g_free (str);
              break;
            }
          g_free (str);
        }
    }

  channel_reconnect_ext_input_ports (ch);
}

static void
on_ext_input_changed (
  TrackInputExpanderWidget * self,
  GtkComboBox *              widget,
  const int                  midi,
  const int                  left)
{
  const char * id =
    gtk_combo_box_get_active_id (widget);

  if (!id)
    return;

  Channel * ch = self->track->channel;
  if (string_is_equal (id, "all", 1))
    {
      if (midi)
        {
          if (ch->all_midi_ins)
            return;
          ch->all_midi_ins = 1;
        }
      else if (left)
        {
          if (ch->all_stereo_l_ins)
            return;
          ch->all_stereo_l_ins = 1;
        }
      else
        {
          if (ch->all_stereo_r_ins)
            return;
          ch->all_stereo_r_ins = 1;
        }
    }
  else if (string_is_equal (id, "none", 1))
    {
      if (midi)
        {
          ch->all_midi_ins = 0;
          ext_ports_free (
            ch->ext_midi_ins,
            ch->num_ext_midi_ins);
          ch->num_ext_midi_ins = 0;
        }
      else if (left)
        {
          ch->all_stereo_l_ins = 0;
          ext_ports_free (
            ch->ext_stereo_l_ins,
            ch->num_ext_stereo_l_ins);
          ch->num_ext_stereo_l_ins = 0;
        }
      else
        {
          ch->all_stereo_r_ins = 0;
          ext_ports_free (
            ch->ext_stereo_r_ins,
            ch->num_ext_stereo_r_ins);
          ch->num_ext_stereo_r_ins = 0;
        }
    }
  else
    {
      if (midi)
        {
          ext_ports_free (
            ch->ext_midi_ins,
            ch->num_ext_midi_ins);
          ch->all_midi_ins = 0;
        }
      else if (left)
        {
          ext_ports_free (
            ch->ext_stereo_l_ins,
            ch->num_ext_stereo_l_ins);
          ch->all_stereo_l_ins = 0;
        }
      else
        {
          ext_ports_free (
            ch->ext_stereo_r_ins,
            ch->num_ext_stereo_r_ins);
          ch->all_stereo_r_ins = 0;
        }

      ExtPort * ports[EXT_PORTS_MAX];
      int       num_ports;
      ext_ports_get (
        midi ? TYPE_EVENT : TYPE_AUDIO,
        FLOW_OUTPUT, 1, ports,
        &num_ports);
      ExtPort * port;
      for (int i = 0; i < num_ports; i++)
        {
          port = ports[i];
          /*g_message ("checking %s",*/
                     /*port->full_name);*/

          if (string_is_equal (
                port->full_name, id, 1))
            {
              if (midi)
                {
                  ch->num_ext_midi_ins = 1;
                  ch->ext_midi_ins[0] =
                    ext_port_clone (port);
                }
              else if (left)
                {
                  ch->num_ext_stereo_l_ins = 1;
                  ch->ext_stereo_l_ins[0] =
                    ext_port_clone (port);
                }
              else
                {
                  ch->num_ext_stereo_r_ins = 1;
                  ch->ext_stereo_r_ins[0] =
                    ext_port_clone (port);
                }
              break;
            }
        }
      ext_ports_free (ports, num_ports);
    }

  channel_reconnect_ext_input_ports (ch);
}

static void
on_midi_input_changed (
  GtkComboBox * widget,
  TrackInputExpanderWidget * self)
{
  on_ext_input_changed (self, widget, 1, 0);
}

static void
on_stereo_l_input_changed (
  GtkComboBox * widget,
  TrackInputExpanderWidget * self)
{
  on_ext_input_changed (self, widget, 0, 1);
}

static void
on_stereo_r_input_changed (
  GtkComboBox * widget,
  TrackInputExpanderWidget * self)
{
  on_ext_input_changed (self, widget, 0, 0);
}

static int
row_separator_func (
  GtkTreeModel *model,
  GtkTreeIter *iter,
  TrackInputExpanderWidget * self)
{
  GValue a = G_VALUE_INIT;
  /*g_value_init (&a, G_TYPE_STRING);*/
  gtk_tree_model_get_value (model, iter, 0, &a);
  const char * val = g_value_get_string (&a);
  int ret;
  if (val && string_is_equal (val, "separator", 1))
    ret = 1;
  else
    ret = 0;
  g_value_unset (&a);
  return ret;
}

/**
 * Sets up external inputs combo box.
 *
 * @param midi 1 for MIDI inputs.
 * @param left If midi is 0, then this indicates
 *   whether to use the left channel (1) or right
 *   (0).
 */
static void
setup_ext_ins_cb (
  TrackInputExpanderWidget * self,
  const int                  midi,
  const int                  left)
{
  GtkComboBox * cb = NULL;
  if (midi)
    cb = self->midi_input;
  else if (left)
    cb = self->stereo_l_input;
  else
    cb = self->stereo_r_input;

  gtk_combo_box_text_remove_all (
    GTK_COMBO_BOX_TEXT (cb));

  if (midi)
    gtk_combo_box_text_append (
      GTK_COMBO_BOX_TEXT (cb),
      "all", _("All MIDI Inputs"));
  else
    gtk_combo_box_text_append (
      GTK_COMBO_BOX_TEXT (cb),
      "all", _("All Audio Inputs"));
  gtk_combo_box_text_append (
    GTK_COMBO_BOX_TEXT (cb),
    "separator", "separator");

  /* get all inputs */
  ExtPort * ports[EXT_PORTS_MAX];
  int       num_ports;
  ext_ports_get (
    midi ? TYPE_EVENT : TYPE_AUDIO,
    FLOW_OUTPUT, 1, ports, &num_ports);

  ExtPort * port;
  char * label;
  for (int i = 0; i < num_ports; i++)
    {
      port = ports[i];

      if (port->num_aliases == 2)
        label = port->alias2;
      else if (port->num_aliases == 1)
        label = port->alias1;
      else if (port->short_name)
        label = port->short_name;
      else
        label = port->full_name;

      gtk_combo_box_text_append (
        GTK_COMBO_BOX_TEXT (cb),
        port->full_name, label);
    }
  ext_ports_free (ports, num_ports);

  gtk_combo_box_text_append (
    GTK_COMBO_BOX_TEXT (cb),
    "separator", "separator");
  if (midi)
    {
      gtk_combo_box_text_append (
        GTK_COMBO_BOX_TEXT (cb),
        "none", _("No inputs"));
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (cb),
        _("MIDI inputs for recording"));
    }
  else
    {
      gtk_combo_box_text_append (
        GTK_COMBO_BOX_TEXT (cb),
        "none",
        left ?
          _("No left input") :
          _("No right input"));
      char * str =
        g_strdup_printf (
          _("Audio input (%s) for recording"),
          /* TRANSLATORS: Left and Right */
          left ? _("L") : _("R"));
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (cb),
        str);
      g_free (str);
    }

  gtk_widget_set_visible (
    GTK_WIDGET (cb), 1);

  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_STRING);

  /* select the correct value */
  Channel * ch = self->track->channel;
  if (midi)
    {
      if (ch->all_midi_ins)
        g_value_set_string (&a, "all");
      else if (ch->num_ext_midi_ins == 0)
        g_value_set_string (&a, "none");
      else
        {
          g_value_set_string (
            &a, ch->ext_midi_ins[0]->full_name);
        }
    }
  else if (left)
    {
      if (ch->all_stereo_l_ins)
        g_value_set_string (&a, "all");
      else if (ch->num_ext_stereo_l_ins == 0)
        g_value_set_string (&a, "none");
      else
        {
          g_value_set_string (
            &a, ch->ext_stereo_l_ins[0]->full_name);
        }
    }
  else
    {
      if (ch->all_stereo_r_ins)
        g_value_set_string (&a, "all");
      else if (ch->num_ext_stereo_r_ins == 0)
        g_value_set_string (&a, "none");
      else
        {
          g_value_set_string (
            &a, ch->ext_stereo_r_ins[0]->full_name);
        }
    }
  g_object_set_property (
    G_OBJECT (cb),
    "active-id",
    &a);

  g_value_unset (&a);
}

static void
setup_midi_channels_cb (
  TrackInputExpanderWidget * self)
{
  GtkComboBox * cb = self->midi_channels;

  gtk_combo_box_text_remove_all (
    GTK_COMBO_BOX_TEXT (cb));

  gtk_combo_box_text_append (
    GTK_COMBO_BOX_TEXT (cb),
    "all", _("All Channels"));
  gtk_combo_box_text_append (
    GTK_COMBO_BOX_TEXT (cb),
    "separator", "separator");

  char * id, * lbl;
  for (int i = 0; i < 16; i++)
    {
      id = g_strdup_printf ("%d", i + 1);
      lbl = g_strdup_printf (_("Channel %s"), id);

      gtk_combo_box_text_append (
        GTK_COMBO_BOX_TEXT (cb),
        id, lbl);

      g_free (id);
      g_free (lbl);
    }

  gtk_combo_box_text_append (
    GTK_COMBO_BOX_TEXT (cb),
    "separator", "separator");
  gtk_combo_box_text_append (
    GTK_COMBO_BOX_TEXT (cb),
    "none", _("No channel"));

  gtk_widget_set_visible (
    GTK_WIDGET (cb), 1);

  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_STRING);

  /* select the correct value */
  Channel * ch = self->track->channel;
  if (ch->all_midi_channels)
    g_value_set_string (&a, "all");
  else
    {
      int found = 0;
      for (int i = 0; i < 16; i++)
        {
          if (ch->midi_channels[i])
            {
              id = g_strdup_printf ("%d", i + 1);
              g_value_set_string (
                &a, id);
              g_free (id);
              found = 1;
              break;
            }
        }
      if (!found)
        g_value_set_string (
          &a, "none");
    }
  g_object_set_property (
    G_OBJECT (cb),
    "active-id",
    &a);

  g_value_unset (&a);

  gtk_widget_set_tooltip_text (
    GTK_WIDGET (cb),
    _("MIDI channel to filter to"));
}


/**
 * Refreshes each field.
 */
void
track_input_expander_widget_refresh (
  TrackInputExpanderWidget * self,
  Track *                    track)
{
  /* TODO */
  g_warn_if_fail (track);
  self->track = track;

  g_warn_if_fail (
    track->type == TRACK_TYPE_MIDI ||
    track->type == TRACK_TYPE_INSTRUMENT ||
    track->type == TRACK_TYPE_AUDIO);

  gtk_widget_set_visible (
    GTK_WIDGET (self->midi_input), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->midi_channels), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->stereo_l_input), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->stereo_r_input), 0);

  if (track->type == TRACK_TYPE_MIDI ||
      track->type == TRACK_TYPE_INSTRUMENT)
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->midi_input), 1);
      gtk_widget_set_visible (
        GTK_WIDGET (self->midi_channels), 1);

      /* refresh model and select appropriate
       * input */
      setup_ext_ins_cb (self, 1, 0);
      setup_midi_channels_cb (self);

      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self),
        "audio-midi");
    }
  else if (track->type == TRACK_TYPE_AUDIO)
    {
      gtk_widget_set_visible (
        GTK_WIDGET (self->stereo_l_input), 1);
      gtk_widget_set_visible (
        GTK_WIDGET (self->stereo_r_input), 1);

      /* refresh model and select appropriate
       * input */
      setup_ext_ins_cb (self, 0, 1);
      setup_ext_ins_cb (self, 0, 0);

      expander_box_widget_set_icon_resource (
        Z_EXPANDER_BOX_WIDGET (self),
        ICON_TYPE_ZRYTHM,
        "audio.svg");
    }
}

/**
 * Sets up the TrackInputExpanderWidget.
 */
void
track_input_expander_widget_setup (
  TrackInputExpanderWidget * self,
  Track *                    track)
{
}

static void
track_input_expander_widget_class_init (
  TrackInputExpanderWidgetClass * klass)
{
}

static void
track_input_expander_widget_init (
  TrackInputExpanderWidget * self)
{
  self->midi_input =
    GTK_COMBO_BOX (gtk_combo_box_text_new ());
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->midi_input));
  gtk_combo_box_set_row_separator_func (
    self->midi_input,
    (GtkTreeViewRowSeparatorFunc)
      row_separator_func,
    self, NULL);
  g_signal_connect (
    self->midi_input, "changed",
    G_CALLBACK (on_midi_input_changed), self);

  self->midi_channels =
    GTK_COMBO_BOX (gtk_combo_box_text_new ());
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->midi_channels));
  gtk_combo_box_set_row_separator_func (
    self->midi_channels,
    (GtkTreeViewRowSeparatorFunc)
      row_separator_func,
    self, NULL);
  g_signal_connect (
    self->midi_channels, "changed",
    G_CALLBACK (on_midi_channels_changed), self);

  /* setup audio inputs */
  self->stereo_l_input =
    GTK_COMBO_BOX (gtk_combo_box_text_new ());
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->stereo_l_input));
  gtk_combo_box_set_row_separator_func (
    self->stereo_l_input,
    (GtkTreeViewRowSeparatorFunc)
      row_separator_func,
    self, NULL);
  g_signal_connect (
    self->stereo_l_input, "changed",
    G_CALLBACK (on_stereo_l_input_changed), self);

  self->stereo_r_input =
    GTK_COMBO_BOX (gtk_combo_box_text_new ());
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->stereo_r_input));
  gtk_combo_box_set_row_separator_func (
    self->stereo_r_input,
    (GtkTreeViewRowSeparatorFunc)
      row_separator_func,
    self, NULL);
  g_signal_connect (
    self->stereo_r_input, "changed",
    G_CALLBACK (on_stereo_r_input_changed), self);

  /* set name and icon */
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self),
    _("Inputs"));
  expander_box_widget_set_orientation (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_ORIENTATION_VERTICAL);

  z_gtk_combo_box_set_ellipsize_mode (
    self->midi_input, PANGO_ELLIPSIZE_END);
  z_gtk_combo_box_set_ellipsize_mode (
    self->stereo_l_input, PANGO_ELLIPSIZE_END);
  z_gtk_combo_box_set_ellipsize_mode (
    self->stereo_r_input, PANGO_ELLIPSIZE_END);
  z_gtk_combo_box_set_ellipsize_mode (
    self->midi_channels, PANGO_ELLIPSIZE_END);
}
