/* dzl-animation.h
 *
 * Copyright (C) 2010-2016 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DZL_ANIMATION_H
#define DZL_ANIMATION_H

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define DZL_TYPE_ANIMATION      (dzl_animation_get_type())
#define DZL_TYPE_ANIMATION_MODE (dzl_animation_mode_get_type())

G_DECLARE_FINAL_TYPE (DzlAnimation, dzl_animation, DZL, ANIMATION, GInitiallyUnowned)

enum _DzlAnimationMode
{
  DZL_ANIMATION_LINEAR,
  DZL_ANIMATION_EASE_IN_QUAD,
  DZL_ANIMATION_EASE_OUT_QUAD,
  DZL_ANIMATION_EASE_IN_OUT_QUAD,
  DZL_ANIMATION_EASE_IN_CUBIC,
  DZL_ANIMATION_EASE_OUT_CUBIC,
  DZL_ANIMATION_EASE_IN_OUT_CUBIC,

  DZL_ANIMATION_LAST
};

typedef enum   _DzlAnimationMode    DzlAnimationMode;

GType         dzl_animation_mode_get_type      (void);
void          dzl_animation_start              (DzlAnimation     *animation);
void          dzl_animation_stop               (DzlAnimation     *animation);
void          dzl_animation_add_property       (DzlAnimation     *animation,
                                                GParamSpec       *pspec,
                                                const GValue     *value);
DzlAnimation *dzl_object_animatev              (gpointer          object,
                                                DzlAnimationMode  mode,
                                                guint             duration_msec,
                                                GdkFrameClock    *frame_clock,
                                                const gchar      *first_property,
                                                va_list           args);
DzlAnimation* dzl_object_animate               (gpointer          object,
                                                DzlAnimationMode  mode,
                                                guint             duration_msec,
                                                GdkFrameClock    *frame_clock,
                                                const gchar      *first_property,
                                                ...) G_GNUC_NULL_TERMINATED;
DzlAnimation* dzl_object_animate_full          (gpointer          object,
                                                DzlAnimationMode  mode,
                                                guint             duration_msec,
                                                GdkFrameClock    *frame_clock,
                                                GDestroyNotify    notify,
                                                gpointer          notify_data,
                                                const gchar      *first_property,
                                                ...) G_GNUC_NULL_TERMINATED;
guint         dzl_animation_calculate_duration (GdkMonitor       *monitor,
                                                gdouble           from_value,
                                                gdouble           to_value);

G_END_DECLS

#endif /* DZL_ANIMATION_H */
