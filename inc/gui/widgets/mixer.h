// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Mixer widget.
 */

#ifndef __GUI_WIDGETS_MIXER_H__
#define __GUI_WIDGETS_MIXER_H__

#include "gui/widgets/main_window.h"

#include <gtk/gtk.h>

typedef struct _DragDestBoxWidget DragDestBoxWidget;
typedef struct Channel            Channel;
typedef struct _ChannelSlotWidget ChannelSlotWidget;
typedef struct Track              Track;
typedef struct _AddTrackMenuButtonWidget
  AddTrackMenuButtonWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MIXER_WIDGET_TYPE \
  (mixer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MixerWidget,
  mixer_widget,
  Z,
  MIXER_WIDGET,
  GtkBox)

#define MW_MIXER MW_BOT_DOCK_EDGE->mixer

typedef struct _MixerWidget
{
  GtkBox parent_instance;

  /** Drag n drop dest box. */
  DragDestBoxWidget * ddbox;

  /**
   * Box containing all channels except master.
   */
  GtkBox * channels_box;

  /**
   * The track where dnd originated from.
   *
   * Used to decide if left/right highlight means
   * the track should be dropped before or after.
   */
  Track * start_drag_track;

  AddTrackMenuButtonWidget * channels_add;
  GtkBox *                   master_box;

  /**
   * Selected slot to paste selections (when
   * copy-pasting plugins).
   */
  ChannelSlotWidget * paste_slot;

  bool setup;
} MixerWidget;

/**
 * To be called once.
 */
void
mixer_widget_setup (
  MixerWidget * self,
  Channel *     master);

/**
 * Deletes and readds all channels.
 *
 * To be used sparingly.
 */
void
mixer_widget_hard_refresh (MixerWidget * self);

/**
 * Calls refresh on each channel.
 */
void
mixer_widget_soft_refresh (MixerWidget * self);

MixerWidget *
mixer_widget_new (void);

/**
 * @}
 */

#endif
