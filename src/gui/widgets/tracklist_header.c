/*
 * gui/widgets/tracklist_header.c - Main window widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/tracklist.h"
#include "gui/widgets/tracklist_header.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  TracklistHeaderWidget,
  tracklist_header_widget,
  GTK_TYPE_GRID)

void
tracklist_header_widget_refresh_track_count (
  TracklistHeaderWidget * self)
{
  char buf[40];
  int  num_visible =
    tracklist_get_num_visible_tracks (TRACKLIST, 1);
  sprintf (
    buf, "%d/%d", num_visible,
    TRACKLIST->num_tracks);
  gtk_label_set_text (self->track_count_lbl, buf);
}

void
tracklist_header_widget_setup (
  TracklistHeaderWidget * self)
{
  tracklist_header_widget_refresh_track_count (
    self);
}

static void
tracklist_header_widget_init (
  TracklistHeaderWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
tracklist_header_widget_class_init (
  TracklistHeaderWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "tracklist_header.ui");

  gtk_widget_class_set_css_name (
    klass, "tracklist-header");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, TracklistHeaderWidget, x)

  BIND_CHILD (track_count_lbl);
}
