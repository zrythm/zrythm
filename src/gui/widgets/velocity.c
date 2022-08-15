// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <math.h>

#include "audio/channel.h"
#include "audio/channel_track.h"
#include "audio/chord_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Draws the Velocity in the given cairo context in
 * relative coordinates.
 */
void
velocity_draw (Velocity * self, GtkSnapshot * snapshot)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  MidiNote *       mn = velocity_get_midi_note (self);
  ArrangerWidget * arranger =
    arranger_object_get_arranger (obj);

  /* get color */
  GdkRGBA color;
  midi_note_get_adjusted_color (mn, &color);

  /* make velocity start at 0,0 to make it easier to
   * draw */
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot,
    &GRAPHENE_POINT_INIT (
      (float) obj->full_rect.x, (float) obj->full_rect.y));

  /* --- draw --- */

  const int circle_radius = obj->full_rect.width / 2;

  /* draw line */
  gtk_snapshot_append_color (
    snapshot, &color,
    &GRAPHENE_RECT_INIT (
      (float) obj->full_rect.width / 2.f
        - VELOCITY_LINE_WIDTH / 2.f,
      (float) circle_radius, VELOCITY_LINE_WIDTH,
      (float) obj->full_rect.height));

  /*
   * draw circle:
   * 1. translate by a little because we add an
   * extra pixel to VELOCITY_WIDTH to mimic previous
   * behavior
   * 2. push a circle clip and fill with white
   * 3. append colored border
   */
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (
    snapshot, &GRAPHENE_POINT_INIT (-0.5f, -0.5f));
  float          circle_angle = 2.f * (float) M_PI;
  GskRoundedRect rounded_rect;
  gsk_rounded_rect_init_from_rect (
    &rounded_rect,
    &GRAPHENE_RECT_INIT (
      0.f, 0.f, (float) circle_radius * 2.f + 1.f,
      (float) circle_radius * 2.f + 1.f),
    circle_angle);
  gtk_snapshot_push_rounded_clip (snapshot, &rounded_rect);
  gtk_snapshot_append_color (
    snapshot, &Z_GDK_RGBA_INIT (0.8, 0.8, 0.8, 1),
    &rounded_rect.bounds);
  const float border_width = 2.f;
  float       border_widths[] = {
          border_width, border_width, border_width, border_width
  };
  GdkRGBA border_colors[] = { color, color, color, color };
  gtk_snapshot_append_border (
    snapshot, &rounded_rect, border_widths, border_colors);
  gtk_snapshot_pop (snapshot);
  gtk_snapshot_restore (snapshot);

  /* draw text */
  if (arranger->action != UI_OVERLAY_ACTION_NONE)
    {
      char text[8];
      sprintf (text, "%d", self->vel);
      const int padding = 3;
      int       text_start_y = padding;
      int       text_start_x = obj->full_rect.width + padding;

      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          (float) text_start_x, (float) text_start_y));
      PangoLayout * layout = arranger->vel_layout;
      pango_layout_set_text (layout, text, -1);
      gtk_snapshot_append_layout (
        snapshot, layout, &Z_GDK_RGBA_INIT (1, 1, 1, 1));
      gtk_snapshot_restore (snapshot);
    }

  gtk_snapshot_restore (snapshot);
}

#if 0
/**
 * Creates a velocity.
 */
VelocityWidget *
velocity_widget_new (Velocity * velocity)
{
  VelocityWidget * self =
    g_object_new (VELOCITY_WIDGET_TYPE,
                  NULL);

  arranger_object_widget_setup (
    Z_ARRANGER_OBJECT_WIDGET (self),
    (ArrangerObject *) velocity);

  self->velocity = velocity;

  return self;
}
#endif
