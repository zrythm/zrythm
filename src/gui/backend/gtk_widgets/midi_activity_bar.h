/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __GUI_WIDGETS_MIDI_ACTIVITY_BAR_H__
#define __GUI_WIDGETS_MIDI_ACTIVITY_BAR_H__

/**
 * @file
 *
 * MIDI activity bar for tracks.
 */

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define MIDI_ACTIVITY_BAR_WIDGET_TYPE (midi_activity_bar_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiActivityBarWidget,
  midi_activity_bar_widget,
  Z,
  MIDI_ACTIVITY_BAR_WIDGET,
  GtkWidget)

class Track;

/**
 * @addtogroup widgets
 *
 * @{
 */

enum class MidiActivityBarType
{
  MAB_TYPE_TRACK,
  MAB_TYPE_ENGINE,
};

enum class MidiActivityBarAnimation
{
  /** Shows a bars that decreases over time. */
  MAB_ANIMATION_BAR,
  /** Shows a flash that fades out over time. */
  MAB_ANIMATION_FLASH,
};

typedef struct _MidiActivityBarWidget
{
  GtkWidget parent_instance;

  /** Track associated with this widget. */
  Track * track;

  MidiActivityBarType type;

  MidiActivityBarAnimation animation;

  /** System time at last trigger, so we know
   * how to draw the bar (for fading down). */
  gint64 last_trigger_time;

  /** Draw border or not. */
  int draw_border;

} MidiActivityBarWidget;

/**
 * Creates a MidiActivityBarWidget for use inside
 * TrackWidget implementations.
 */
void
midi_activity_bar_widget_setup_track (
  MidiActivityBarWidget * self,
  Track *                 track);

/**
 * Sets the animation.
 */
void
midi_activity_bar_widget_set_animation (
  MidiActivityBarWidget *  self,
  MidiActivityBarAnimation animation);

/**
 * Creates a MidiActivityBarWidget for the
 * AudioEngine.
 */
void
midi_activity_bar_widget_setup_engine (MidiActivityBarWidget * self);

/**
 * @}
 */

#endif
