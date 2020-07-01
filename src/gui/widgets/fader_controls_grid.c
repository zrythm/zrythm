/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/track.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/fader_controls_expander.h"
#include "gui/widgets/fader_controls_grid.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/meter.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  FaderControlsGridWidget,
  fader_controls_grid_widget,
  GTK_TYPE_GRID)

static bool
update_meter_reading (
  FaderControlsGridWidget * widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  if (!MAIN_WINDOW ||
      !gtk_widget_get_mapped (
        GTK_WIDGET (widget)) ||
      !widget->track)
    {
      return G_SOURCE_CONTINUE;
    }

  if (widget != MW_TRACK_INSPECTOR->fader->grid)
    {
      return G_SOURCE_REMOVE;
    }

  double prev = widget->meter_reading_val;
  Track * track = widget->track;
  Channel * channel =
    track_get_channel (widget->track);
  g_warn_if_fail (channel);

  if (track->out_signal_type == TYPE_EVENT)
    {
      gtk_label_set_text (
        widget->meter_readings, "-∞");
      return G_SOURCE_CONTINUE;
    }

  float amp =
    MAX (
      widget->meter_l->meter->prev_max,
      widget->meter_r->meter->prev_max);

  double peak_val = (double) math_amp_to_dbfs (amp);

  if (math_doubles_equal (peak_val, prev))
    return G_SOURCE_CONTINUE;
  if (peak_val < -98.)
    gtk_label_set_text (
      widget->meter_readings, "-∞");
  else
    {
      char format_str[400];
      strcpy (format_str, _("Peak"));
      strcat (format_str, ":\n<small>");
      if (peak_val < -10.)
        {
          strcat (format_str, "%.0fdb</small>");
        }
      else
        {
          if (peak_val > 0)
            {
              strcat (
                format_str,
                "<span foreground=\"#FF0A05\">"
                "%.1fdb</span></small>");
            }
          else
            {
              strcat (format_str, "%.1fdb</small>");
            }
        }
      char str[800];
      sprintf (str, format_str, peak_val);
      gtk_label_set_markup (
        widget->meter_readings, str);
    }

  widget->meter_reading_val = peak_val;

  return G_SOURCE_CONTINUE;
}

static void
on_record_toggled (
  GtkToggleButton * btn,
  FaderControlsGridWidget * self)
{
  if (self->track)
    {
      track_set_recording (
        self->track,
        gtk_toggle_button_get_active (btn), 1);
    }
}

static void
on_solo_toggled (
  GtkToggleButton * btn,
  FaderControlsGridWidget * self)
{
  if (self->track)
    {
      track_set_soloed (
        self->track,
        gtk_toggle_button_get_active (btn),
        true, true);
    }
}

static void
on_mute_toggled (
  GtkToggleButton * btn,
  FaderControlsGridWidget * self)
{
  if (self->track)
    {
      track_set_muted (
        self->track,
        gtk_toggle_button_get_active (btn),
        true, true);
    }
}

static void
setup_balance_control (
  FaderControlsGridWidget * self)
{
  z_gtk_container_destroy_all_children (
    GTK_CONTAINER (self->balance_box));

  if (!self->track)
    return;

  if (track_type_has_channel (self->track->type))
    {
      Channel * ch = track_get_channel (self->track);
      self->balance_control =
        balance_control_widget_new (
          channel_get_balance_control,
          channel_set_balance_control,
          ch, 12);
      gtk_box_pack_start (
        self->balance_box,
        GTK_WIDGET (self->balance_control),
        Z_GTK_NO_EXPAND, Z_GTK_FILL, 0);
      gtk_widget_set_hexpand (
        GTK_WIDGET (self->balance_control), true);
      z_gtk_widget_set_margin (
        GTK_WIDGET (self->balance_control), 4);
    }
}

static void
setup_meter (
  FaderControlsGridWidget * self)
{
  Track * track = self->track;
  if (!track)
    return;

  Channel * ch = NULL;
  if (track_type_has_channel (track->type))
    ch = track_get_channel (track);
  switch (track->out_signal_type)
    {
    case TYPE_EVENT:
      meter_widget_setup (
        self->meter_l,
        ch->midi_out, 14);
      gtk_widget_set_margin_start (
        GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_margin_end (
        GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_visible (
        GTK_WIDGET (self->meter_r), false);
      break;
    case TYPE_AUDIO:
      meter_widget_setup (
        self->meter_l,
        ch->stereo_out->l, 12);
      meter_widget_setup (
        self->meter_r,
        ch->stereo_out->r, 12);
      gtk_widget_set_visible (
        GTK_WIDGET (self->meter_r), true);
      break;
    default:
      break;
    }
}

static void
setup_fader (
  FaderControlsGridWidget * self)
{
  if (track_type_has_channel (self->track->type))
    {
      Channel * ch =
        track_get_channel (self->track);
      fader_widget_setup (
        self->fader, ch->fader, 36, 128);
      gtk_widget_set_margin_start (
        GTK_WIDGET (self->fader), 12);
      gtk_widget_set_halign (
        GTK_WIDGET (self->fader), GTK_ALIGN_CENTER);
    }
}

void
fader_controls_grid_widget_block_signal_handlers (
  FaderControlsGridWidget * self)
{
  g_signal_handler_block (
    self->solo,
    self->solo_toggled_handler_id);
  g_signal_handler_block (
    self->mute,
    self->mute_toggled_handler_id);
}

void
fader_controls_grid_widget_unblock_signal_handlers (
  FaderControlsGridWidget * self)
{
  g_signal_handler_unblock (
    self->solo,
    self->solo_toggled_handler_id);
  g_signal_handler_unblock (
    self->mute,
    self->mute_toggled_handler_id);
}

void
fader_controls_grid_widget_setup (
  FaderControlsGridWidget * self,
  Track *                   track)
{
  self->track = track;

  setup_fader (self);
  setup_meter (self);
  setup_balance_control (self);

  if (track)
    {
      fader_controls_grid_widget_block_signal_handlers (
        self);
      gtk_toggle_button_set_active (
        self->mute, track_get_muted (track));
      if (track_type_can_record (track->type))
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self->record), true);
          gtk_toggle_button_set_active (
            self->record, track->recording);
        }
      else
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self->record), false);
        }
      gtk_toggle_button_set_active (
        self->solo, track_get_soloed (track));
      fader_controls_grid_widget_unblock_signal_handlers (
        self);
    }
}

/**
 * Prepare for finalization.
 */
void
fader_controls_grid_widget_tear_down (
  FaderControlsGridWidget * self)
{
  g_message ("tearing down %p...", self);

  if (self->tick_cb)
    {
      g_message ("removing tick callback...");
      gtk_widget_remove_tick_callback (
        GTK_WIDGET (self), self->tick_cb);
      self->tick_cb = 0;
    }

  g_message ("done");
}

FaderControlsGridWidget *
fader_controls_grid_widget_new (void)
{
  g_message ("creating...");

  FaderControlsGridWidget * self =
    g_object_new (
      FADER_CONTROLS_GRID_WIDGET_TYPE, NULL);

  self->tick_cb =
    gtk_widget_add_tick_callback (
      GTK_WIDGET (self),
      (GtkTickCallback) update_meter_reading,
      self, NULL);

  g_message ("done");

  return self;
}

static void
fader_controls_grid_finalize (
  FaderControlsGridWidget * self)
{
  g_message ("finalizing...");

  fader_controls_grid_widget_tear_down (self);

  G_OBJECT_CLASS (
    fader_controls_grid_widget_parent_class)->
      finalize (G_OBJECT (self));

  g_message ("done");
}

static void
fader_controls_grid_widget_init (
  FaderControlsGridWidget * self)
{
  g_type_ensure (FADER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_size_request (
    GTK_WIDGET (self->meter_readings), 50, -1);

  /* add css classes */
  GtkStyleContext * context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->record));
  gtk_style_context_add_class (
    context, "record-button");
  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self->solo));
  gtk_style_context_add_class (
    context, "solo-button");

  self->solo_toggled_handler_id =
    g_signal_connect (
      G_OBJECT (self->solo), "toggled",
      G_CALLBACK (on_solo_toggled), self);
  self->mute_toggled_handler_id =
    g_signal_connect (
      G_OBJECT (self->mute), "toggled",
      G_CALLBACK (on_mute_toggled), self);
  self->record_toggled_handler_id =
    g_signal_connect (
      G_OBJECT (self->record), "toggled",
      G_CALLBACK (on_record_toggled), self);
}

static void
fader_controls_grid_widget_class_init (
  FaderControlsGridWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "fader_controls_grid.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, FaderControlsGridWidget, x)

  BIND_CHILD (meters_box);
  BIND_CHILD (balance_box);
  BIND_CHILD (fader);
  BIND_CHILD (fader_buttons);
  BIND_CHILD (meter_l);
  BIND_CHILD (meter_r);
  BIND_CHILD (solo);
  BIND_CHILD (mute);
  BIND_CHILD (record);
  BIND_CHILD (meter_readings);

#undef BIND_CHILD

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc)
    fader_controls_grid_finalize;
}
