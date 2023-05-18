// SPDX-FileCopyrightText: © 2018-2019, 2022-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/tracklist.h"
#include "gui/widgets/popovers/track_filter_popover.h"
#include "gui/widgets/popovers/tracklist_preferences_popover.h"
#include "gui/widgets/tracklist_header.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  TracklistHeaderWidget,
  tracklist_header_widget,
  GTK_TYPE_WIDGET)

void
tracklist_header_widget_refresh_track_count (
  TracklistHeaderWidget * self)
{
  char buf[40];
  int  num_visible =
    tracklist_get_num_visible_tracks (TRACKLIST, 1);
  sprintf (buf, "%d/%d", num_visible, TRACKLIST->num_tracks);
  gtk_label_set_text (self->track_count_lbl, buf);
}

void
tracklist_header_widget_setup (TracklistHeaderWidget * self)
{
  tracklist_header_widget_refresh_track_count (self);
}

static void
create_filter_popup (
  GtkMenuButton * menu_btn,
  gpointer        user_data)
{
  TrackFilterPopoverWidget * track_filter_popover =
    track_filter_popover_widget_new ();
  gtk_menu_button_set_popover (
    menu_btn, GTK_WIDGET (track_filter_popover));
}

static void
create_tracklist_preferences_popup (
  GtkMenuButton * menu_btn,
  gpointer        user_data)
{
  TracklistPreferencesPopoverWidget * tracklist_pref_popover =
    tracklist_preferences_popover_widget_new ();
  gtk_menu_button_set_popover (
    menu_btn, GTK_WIDGET (tracklist_pref_popover));
}

static void
tracklist_header_widget_init (TracklistHeaderWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_menu_button_set_create_popup_func (
    self->filter_menu_btn, create_filter_popup, self, NULL);
  gtk_menu_button_set_create_popup_func (
    self->tracklist_pref_btn,
    create_tracklist_preferences_popup, self, NULL);

  /* hack - this will cause the tracklist to get filtered */
  TrackFilterPopoverWidget * track_filter_popover =
    track_filter_popover_widget_new ();
  g_object_ref_sink (track_filter_popover);
  g_object_unref (track_filter_popover);
}

static void
tracklist_header_widget_class_init (
  TracklistHeaderWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "tracklist_header.ui");

  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (klass, "tracklist-header");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, TracklistHeaderWidget, x)

  BIND_CHILD (track_count_lbl);
  BIND_CHILD (filter_menu_btn);
  BIND_CHILD (tracklist_pref_btn);

#undef BIND_CHILD
}
