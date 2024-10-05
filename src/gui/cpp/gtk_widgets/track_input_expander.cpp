// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel_track.h"
#include "gui/cpp/gtk_widgets/ext_input_selection_dropdown.h"
#include "gui/cpp/gtk_widgets/knob.h"
#include "gui/cpp/gtk_widgets/midi_channel_selection_dropdown.h"
#include "gui/cpp/gtk_widgets/track_input_expander.h"
#include "project.h"
#include "utils/gtk.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  TrackInputExpanderWidget,
  track_input_expander_widget,
  TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

/**
 * Sets up external inputs dropdown.
 *
 * @param midi 1 for MIDI inputs.
 * @param left If midi is false, then this indicates whether to use the left
 * channel (true) or right (false).
 */
static void
setup_ext_ins_dropdown (
  TrackInputExpanderWidget * self,
  const bool                 midi,
  const bool                 left)
{
  GtkDropDown * dropdown = nullptr;
  if (midi)
    dropdown = self->midi_input;
  else if (left)
    dropdown = self->stereo_l_input;
  else
    dropdown = self->stereo_r_input;

  ext_input_selection_dropdown_widget_refresh (dropdown, self->track, left);

  if (midi)
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (dropdown), _ ("MIDI inputs for recording"));
    }
  else
    {
      char * str = g_strdup_printf (
        _ ("Audio input (%s) for recording"),
        /* TRANSLATORS: Left and Right */
        left ? _ ("L") : _ ("R"));
      gtk_widget_set_tooltip_text (GTK_WIDGET (dropdown), str);
      g_free (str);
    }

  gtk_widget_set_visible (GTK_WIDGET (dropdown), true);
}

static void
setup_midi_channels_dropdown (TrackInputExpanderWidget * self)
{
  GtkDropDown * dropdown = self->midi_channels;

  midi_channel_selection_dropdown_widget_refresh (dropdown, self->track);

  gtk_widget_set_visible (GTK_WIDGET (dropdown), true);

  gtk_widget_set_tooltip_text (
    GTK_WIDGET (dropdown), _ ("MIDI channel to filter to"));
}

static void
on_mono_toggled (GtkToggleButton * btn, TrackInputExpanderWidget * self)
{
  if (!self->track || !self->track->is_audio ())
    return;

  bool new_val = gtk_toggle_button_get_active (btn);
  self->track->processor_->mono_->set_val_from_normalized (new_val, false);
  gtk_widget_set_sensitive (GTK_WIDGET (self->stereo_r_input), !new_val);
}

/**
 * Refreshes each field.
 */
void
track_input_expander_widget_refresh (
  TrackInputExpanderWidget * self,
  ChannelTrack *             track)
{
  z_return_if_fail (track);
  self->track = track;

  gtk_widget_set_visible (GTK_WIDGET (self->midi_input), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->midi_channels), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->stereo_l_input), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->stereo_r_input), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->gain_box), 0);
  gtk_widget_set_visible (GTK_WIDGET (self->mono), 0);
  if (self->gain)
    {
      gtk_box_remove (GTK_BOX (self->gain_box), GTK_WIDGET (self->gain));
      self->gain = NULL;
    }

  if (!track->is_midi () && !track->is_instrument () && !track->is_audio ())
    {
      return;
    }

  if (track->has_piano_roll ())
    {
      gtk_widget_set_visible (GTK_WIDGET (self->midi_input), 1);
      gtk_widget_set_visible (GTK_WIDGET (self->midi_channels), 1);

      /* refresh model and select appropriate
       * input */
      setup_ext_ins_dropdown (self, true, false);
      setup_midi_channels_dropdown (self);

      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self), "midi-insert");
    }
  else if (track->is_audio ())
    {
      gtk_widget_set_visible (GTK_WIDGET (self->stereo_l_input), 1);
      gtk_widget_set_visible (GTK_WIDGET (self->stereo_r_input), 1);

      /* refresh model and select appropriate
       * input */
      setup_ext_ins_dropdown (self, 0, 1);
      setup_ext_ins_dropdown (self, 0, 0);

      auto &port = track->processor_->input_gain_;
      self->gain = knob_widget_new_simple (
        bind_member_function (*port, &ControlPort::get_val),
        bind_member_function (*port, &ControlPort::get_default_val),
        bind_member_function (*port, &ControlPort::set_real_val_w_events),
        port.get (), port->minf_, port->maxf_, 24, port->zerof_);
      gtk_box_append (GTK_BOX (self->gain_box), GTK_WIDGET (self->gain));

      gtk_toggle_button_set_active (
        self->mono, track->processor_->mono_->is_toggled ());

      gtk_widget_set_visible (GTK_WIDGET (self->mono), true);
      gtk_widget_set_visible (GTK_WIDGET (self->gain_box), true);

      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self), "audio-insert");
    }
}

/**
 * Sets up the TrackInputExpanderWidget.
 */
void
track_input_expander_widget_setup (
  TrackInputExpanderWidget * self,
  ChannelTrack *             track)
{
}

static void
track_input_expander_widget_class_init (TrackInputExpanderWidgetClass * klass)
{
}

static void
track_input_expander_widget_init (TrackInputExpanderWidget * self)
{
  self->midi_input = GTK_DROP_DOWN (gtk_drop_down_new (nullptr, nullptr));
  gtk_widget_set_hexpand (GTK_WIDGET (self->midi_input), true);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->midi_input));

  self->midi_channels = GTK_DROP_DOWN (gtk_drop_down_new (nullptr, nullptr));
  gtk_widget_set_hexpand (GTK_WIDGET (self->midi_channels), true);
  two_col_expander_box_widget_add_single (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->midi_channels));

  self->audio_input_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* setup audio inputs */
  self->stereo_l_input = GTK_DROP_DOWN (gtk_drop_down_new (nullptr, nullptr));
  gtk_widget_set_hexpand (GTK_WIDGET (self->stereo_l_input), true);
  gtk_size_group_add_widget (
    self->audio_input_size_group, GTK_WIDGET (self->stereo_l_input));
  self->mono = z_gtk_toggle_button_new_with_icon ("codicons-merge");
  gtk_widget_set_visible (GTK_WIDGET (self->mono), true);
  g_signal_connect (self->mono, "toggled", G_CALLBACK (on_mono_toggled), self);
  two_col_expander_box_widget_add_pair (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->stereo_l_input),
    GTK_WIDGET (self->mono));
  GtkWidget * parent_box = gtk_widget_get_parent (GTK_WIDGET (self->mono));
  (void) parent_box;
  /*gtk_box_set_child_packing (*/
  /*GTK_BOX (parent_box),*/
  /*GTK_WIDGET (self->mono), F_NO_EXPAND,*/
  /*F_FILL, 1, GTK_PACK_START);*/
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->mono), _ ("Mono"));

  self->stereo_r_input = GTK_DROP_DOWN (gtk_drop_down_new (nullptr, nullptr));
  gtk_widget_set_hexpand (GTK_WIDGET (self->stereo_r_input), true);
  gtk_size_group_add_widget (
    self->audio_input_size_group, GTK_WIDGET (self->stereo_r_input));
  self->gain_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_widget_set_visible (GTK_WIDGET (self->gain_box), true);
  two_col_expander_box_widget_add_pair (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->stereo_r_input),
    GTK_WIDGET (self->gain_box));
  parent_box = gtk_widget_get_parent (GTK_WIDGET (self->gain_box));
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->gain_box), _ ("Gain"));

  /* set name and icon */
  expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), _ ("Inputs"));
  expander_box_widget_set_orientation (
    Z_EXPANDER_BOX_WIDGET (self), GTK_ORIENTATION_VERTICAL);

  /* add css classes */
  gtk_widget_add_css_class (GTK_WIDGET (self), "track-input-expander");
}
