// SPDX-FileCopyrightText: © 2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/channel_slot.h"
#include "gui/cpp/gtk_widgets/fader_controls_expander.h"
#include "gui/cpp/gtk_widgets/fader_controls_grid.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include <glib/gi18n.h>

#include "common/dsp/channel.h"
#include "common/dsp/track.h"
#include "common/utils/gtk.h"

#define FADER_CONTROLS_EXPANDER_WIDGET_TYPE \
  (fader_controls_expander_widget_get_type ())
G_DEFINE_TYPE (
  FaderControlsExpanderWidget,
  fader_controls_expander_widget,
  EXPANDER_BOX_WIDGET_TYPE)

/**
 * Refreshes each field.
 */
void
fader_controls_expander_widget_refresh (FaderControlsExpanderWidget * self)
{
}

/**
 * Sets up the FaderControlsExpanderWidget.
 */
void
fader_controls_expander_widget_setup (
  FaderControlsExpanderWidget * self,
  ChannelTrack *                track)
{
  /* set name and icon */
  expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), _ ("Fader"));

  self->track = track;

  fader_controls_grid_widget_setup (self->grid, track);

  fader_controls_expander_widget_refresh (self);
}

/**
 * Prepare for finalization.
 */
void
fader_controls_expander_widget_tear_down (FaderControlsExpanderWidget * self)
{
  fader_controls_grid_widget_tear_down (self->grid);
}

static void
fader_controls_expander_widget_class_init (
  FaderControlsExpanderWidgetClass * klass)
{
}

static void
fader_controls_expander_widget_init (FaderControlsExpanderWidget * self)
{
  self->grid = fader_controls_grid_widget_new ();
  gtk_widget_set_visible (GTK_WIDGET (self->grid), 1);

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->grid));

  expander_box_widget_set_icon_name (Z_EXPANDER_BOX_WIDGET (self), "fader");

  /* add css classes */
  gtk_widget_add_css_class (GTK_WIDGET (self), "fader-controls-expander");
}
