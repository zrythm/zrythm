/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/control_port.h"
#include "audio/engine.h"
#include "audio/ext_port.h"
#include "audio/hardware_processor.h"
#include "audio/track.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/track_input_expander.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/string.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  TrackInputExpanderWidget,
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
  if (string_is_equal (id, "all"))
    {
      if (ch->all_midi_channels)
        return;
      ch->all_midi_channels = 1;
    }
  else if (string_is_equal (id, "none"))
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
          if (string_is_equal (str, id))
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

  g_debug ("ext input: %s", id);

  if (!id)
    return;

  Channel * ch = self->track->channel;
  if (string_is_equal (id, "all"))
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
  else if (string_is_equal (id, "none"))
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

      /* get all inputs */
      for (int i = 0;
           i <
            (midi?
              HW_IN_PROCESSOR->num_ext_midi_ports :
              HW_IN_PROCESSOR->num_ext_audio_ports);
           i++)
        {
          ExtPort * port =
            midi?
              HW_IN_PROCESSOR->ext_midi_ports[i] :
              HW_IN_PROCESSOR->ext_audio_ports[i];

          char * port_id = ext_port_get_id (port);
          if (string_is_equal (port_id, id))
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
  if (val && string_is_equal (val, "separator"))
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
  for (int i = 0;
       i <
         (midi?
           HW_IN_PROCESSOR->num_ext_midi_ports :
           HW_IN_PROCESSOR->num_ext_audio_ports);
       i++)
    {
      ExtPort * port =
        midi?
          HW_IN_PROCESSOR->ext_midi_ports[i] :
          HW_IN_PROCESSOR->ext_audio_ports[i];

      char * label;
      if (port->num_aliases == 2)
        label = port->alias2;
      else if (port->num_aliases == 1)
        label = port->alias1;
      else if (port->short_name)
        label = port->short_name;
      else
        label = port->full_name;

      /* only use active ports. only enabled
       * ports will be active */
      if (!port->active)
        continue;

      char * port_id = ext_port_get_id (port);
      gtk_combo_box_text_append (
        GTK_COMBO_BOX_TEXT (cb),
        port_id, label);
      g_free (port_id);
    }

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
          char * port_id =
            ext_port_get_id (ch->ext_midi_ins[0]);
          g_value_set_string (&a, port_id);
          g_free (port_id);
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
          char * port_id =
            ext_port_get_id (
              ch->ext_stereo_l_ins[0]);
          g_value_set_string (&a, port_id);
          g_free (port_id);
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
          char * port_id =
            ext_port_get_id (
              ch->ext_stereo_r_ins[0]);
          g_value_set_string (&a, port_id);
          g_free (port_id);
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

static void
on_mono_toggled (
  GtkToggleButton *          btn,
  TrackInputExpanderWidget * self)
{
  if (!self->track ||
      self->track->type != TRACK_TYPE_AUDIO)
    return;

  bool new_val = gtk_toggle_button_get_active (btn);
  control_port_set_val_from_normalized (
    self->track->processor->mono, new_val, false);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->stereo_r_input), !new_val);
}

/**
 * Refreshes each field.
 */
void
track_input_expander_widget_refresh (
  TrackInputExpanderWidget * self,
  Track *                    track)
{
  g_return_if_fail (track);
  self->track = track;

  gtk_widget_set_visible (
    GTK_WIDGET (self->midi_input), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->midi_channels), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->stereo_l_input), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->stereo_r_input), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->gain_box), 0);
  gtk_widget_set_visible (
    GTK_WIDGET (self->mono), 0);
  if (self->gain)
    {
      gtk_box_remove (
        GTK_BOX (self->gain_box),
        GTK_WIDGET (self->gain));
      self->gain = NULL;
    }

  if (
    track->type != TRACK_TYPE_MIDI
    && track->type != TRACK_TYPE_INSTRUMENT
    && track->type != TRACK_TYPE_AUDIO)
    {
      return;
    }

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
        "midi-insert");
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

      Port * port = track->processor->input_gain;
      self->gain =
        knob_widget_new_simple (
          control_port_get_val,
          control_port_get_default_val,
          control_port_set_real_val_w_events,
          port, port->minf, port->maxf, 24,
          port->zerof);
      gtk_box_append (
        GTK_BOX (self->gain_box),
        GTK_WIDGET (self->gain));

      gtk_toggle_button_set_active (
        self->mono,
        control_port_is_toggled (
          track->processor->mono));

      gtk_widget_set_visible (
        GTK_WIDGET (self->mono), true);
      gtk_widget_set_visible (
        GTK_WIDGET (self->gain_box), true);

      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self),
        "audio-insert");
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

  self->audio_input_size_group =
    gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* setup audio inputs */
  self->stereo_l_input =
    GTK_COMBO_BOX (gtk_combo_box_text_new ());
  gtk_size_group_add_widget (
    self->audio_input_size_group,
    GTK_WIDGET (self->stereo_l_input));
  self->mono =
    z_gtk_toggle_button_new_with_icon ("mono");
  gtk_widget_set_visible (
    GTK_WIDGET (self->mono), true);
  g_signal_connect (
    self->mono, "toggled",
    G_CALLBACK (on_mono_toggled), self);
  two_col_expander_box_widget_add_pair (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->stereo_l_input),
    GTK_WIDGET (self->mono));
  GtkWidget * parent_box =
    gtk_widget_get_parent (
      GTK_WIDGET (self->mono));
  (void) parent_box;
  /*gtk_box_set_child_packing (*/
    /*GTK_BOX (parent_box),*/
    /*GTK_WIDGET (self->mono), F_NO_EXPAND,*/
    /*F_FILL, 1, GTK_PACK_START);*/
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->mono), _("Mono"));
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
  gtk_size_group_add_widget (
    self->audio_input_size_group,
    GTK_WIDGET (self->stereo_r_input));
  self->gain_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_widget_set_visible (
    GTK_WIDGET (self->gain_box), true);
  two_col_expander_box_widget_add_pair (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->stereo_r_input),
    GTK_WIDGET (self->gain_box));
  parent_box =
    gtk_widget_get_parent (
      GTK_WIDGET (self->gain_box));
  /*gtk_box_set_child_packing (*/
    /*GTK_BOX (parent_box),*/
    /*GTK_WIDGET (self->gain_box), F_NO_EXPAND,*/
    /*F_FILL, 2, GTK_PACK_START);*/
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->gain_box), _("Gain"));
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

  /* add css classes */
  GtkStyleContext * context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self));
  gtk_style_context_add_class (
    context, "track-input-expander");
}
