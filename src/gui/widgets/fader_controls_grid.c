// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/meter.h"
#include "dsp/track.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/fader_buttons.h"
#include "gui/widgets/fader_controls_expander.h"
#include "gui/widgets/fader_controls_grid.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/meter.h"
#include "project.h"
#include "utils/flags.h"
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
  GdkFrameClock *           frame_clock,
  gpointer                  user_data)
{
  g_return_val_if_fail (
    GTK_IS_WIDGET (widget), G_SOURCE_REMOVE);

  if (
    !MAIN_WINDOW
    || !gtk_widget_get_mapped (GTK_WIDGET (widget))
    || !widget->track)
    {
      return G_SOURCE_CONTINUE;
    }

  if (widget != MW_TRACK_INSPECTOR->fader->grid)
    {
      return G_SOURCE_REMOVE;
    }

  double  prev = widget->meter_reading_val;
  Track * track = widget->track;
  g_warn_if_fail (IS_TRACK (track));
  Channel * channel = track_get_channel (widget->track);
  g_warn_if_fail (channel);

  if (track->out_signal_type == TYPE_EVENT)
    {
      gtk_label_set_text (widget->meter_readings, "-∞");
      return G_SOURCE_CONTINUE;
    }

  float amp = MAX (
    widget->meter_l->meter->prev_max,
    widget->meter_r->meter->prev_max);

  double peak_val = (double) math_amp_to_dbfs (amp);

  if (math_doubles_equal (peak_val, prev))
    return G_SOURCE_CONTINUE;

  char db_str[60];
  ui_get_db_value_as_string ((float) peak_val, db_str);

  if (peak_val < -98.)
    {
      gtk_label_set_text (widget->meter_readings, db_str);
    }
  else
    {
      char str[800];
      strcpy (str, _ ("Peak"));
      strcat (str, ":\n<small>");
      char peak_str[400];
      if (peak_val < -10.)
        {
          sprintf (peak_str, "%sdb</small>", db_str);
        }
      else
        {
          if (peak_val > 0)
            {
              sprintf (
                peak_str,
                "<span foreground=\"#FF0A05\">"
                "%sdb</span></small>",
                db_str);
            }
          else
            {
              sprintf (peak_str, "%sdb</small>", db_str);
            }
        }
      strcat (str, peak_str);
      gtk_label_set_markup (widget->meter_readings, str);
    }

  widget->meter_reading_val = peak_val;

  return G_SOURCE_CONTINUE;
}

static void
setup_balance_control (FaderControlsGridWidget * self)
{
  z_gtk_widget_destroy_all_children (
    GTK_WIDGET (self->balance_box));

  if (!self->track)
    return;

  if (track_type_has_channel (self->track->type))
    {
      Channel * ch = track_get_channel (self->track);
      g_return_if_fail (IS_CHANNEL_AND_NONNULL (ch));
      self->balance_control = balance_control_widget_new (
        channel_get_balance_control,
        channel_set_balance_control, ch, ch->fader->balance,
        12);
      gtk_box_append (
        self->balance_box, GTK_WIDGET (self->balance_control));
      gtk_widget_set_hexpand (
        GTK_WIDGET (self->balance_control), true);
      z_gtk_widget_set_margin (
        GTK_WIDGET (self->balance_control), 4);
    }
}

static void
setup_meter (FaderControlsGridWidget * self)
{
  Track * track = self->track;
  if (!track)
    return;

  Channel * ch = NULL;
  if (track_type_has_channel (track->type))
    {
      ch = track_get_channel (track);
    }
  g_return_if_fail (ch);

  switch (track->out_signal_type)
    {
    case TYPE_EVENT:
      meter_widget_setup (self->meter_l, ch->midi_out, 14);
      gtk_widget_set_margin_start (
        GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_margin_end (
        GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_visible (
        GTK_WIDGET (self->meter_r), false);
      break;
    case TYPE_AUDIO:
      meter_widget_setup (
        self->meter_l, ch->stereo_out->l, 12);
      meter_widget_setup (
        self->meter_r, ch->stereo_out->r, 12);
      gtk_widget_set_visible (
        GTK_WIDGET (self->meter_r), true);
      break;
    default:
      break;
    }
}

static void
setup_fader (FaderControlsGridWidget * self)
{
  if (track_type_has_channel (self->track->type))
    {
      Channel * ch = track_get_channel (self->track);
      g_return_if_fail (IS_CHANNEL_AND_NONNULL (ch));
      fader_widget_setup (self->fader, ch->fader, 36, 128);
      gtk_widget_set_margin_start (
        GTK_WIDGET (self->fader), 12);
      gtk_widget_set_halign (
        GTK_WIDGET (self->fader), GTK_ALIGN_CENTER);
    }
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
  fader_buttons_widget_refresh (self->fader_buttons, track);
}

/**
 * Prepare for finalization.
 */
void
fader_controls_grid_widget_tear_down (
  FaderControlsGridWidget * self)
{
  g_debug ("tearing down %p...", self);

  if (self->tick_cb)
    {
      g_debug ("removing tick callback...");
      gtk_widget_remove_tick_callback (
        GTK_WIDGET (self), self->tick_cb);
      self->tick_cb = 0;
    }

  g_debug ("done");
}

FaderControlsGridWidget *
fader_controls_grid_widget_new (void)
{
  FaderControlsGridWidget * self =
    g_object_new (FADER_CONTROLS_GRID_WIDGET_TYPE, NULL);

  self->tick_cb = gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) update_meter_reading,
    self, NULL);

  return self;
}

static void
fader_controls_grid_finalize (FaderControlsGridWidget * self)
{
  fader_controls_grid_widget_tear_down (self);

  G_OBJECT_CLASS (fader_controls_grid_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
fader_controls_grid_widget_init (
  FaderControlsGridWidget * self)
{
  g_type_ensure (FADER_WIDGET_TYPE);
  g_type_ensure (FADER_BUTTONS_WIDGET_TYPE);
  g_type_ensure (METER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_size_request (
    GTK_WIDGET (self->meter_readings), 50, -1);
}

static void
fader_controls_grid_widget_class_init (
  FaderControlsGridWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
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
  BIND_CHILD (meter_readings);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) fader_controls_grid_finalize;
}
