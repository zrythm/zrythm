// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string.h>

#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/widgets/popovers/tracklist_preferences_popover.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "zrythm.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  TracklistPreferencesPopoverWidget,
  tracklist_preferences_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_auto_arm_switch_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  bool active = gtk_switch_get_active (GTK_SWITCH (gobject));
  g_settings_set_boolean (S_UI, "track-autoarm", active);
}

TracklistPreferencesPopoverWidget *
tracklist_preferences_popover_widget_new (void)
{
  TracklistPreferencesPopoverWidget * self =
    g_object_new (TRACKLIST_PREFERENCES_POPOVER_WIDGET_TYPE, NULL);

  return self;
}

static void
tracklist_preferences_popover_widget_class_init (
  TracklistPreferencesPopoverWidgetClass * _klass)
{
  /*GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);*/
}

static void
tracklist_preferences_popover_widget_init (
  TracklistPreferencesPopoverWidget * self)
{
  AdwPreferencesPage * ppage =
    ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
  /*adw_preferences_page_set_title (*/
  /*ppage, _("Tracklist Preferences"));*/
  gtk_popover_set_child (GTK_POPOVER (self), GTK_WIDGET (ppage));

  {
    AdwPreferencesGroup * pgroup =
      ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
    adw_preferences_group_set_title (pgroup, _ ("Behavior"));
    adw_preferences_page_add (ppage, pgroup);

    {
      AdwActionRow * switch_row = ADW_ACTION_ROW (adw_action_row_new ());
      adw_preferences_row_set_title (
        ADW_PREFERENCES_ROW (switch_row), _ ("Auto-arm for Recording"));
      adw_action_row_set_subtitle (
        ADW_ACTION_ROW (switch_row),
        _ ("Arm tracks for recording when clicked/selected."));
      GtkSwitch * s = GTK_SWITCH (gtk_switch_new ());
      gtk_switch_set_active (s, g_settings_get_boolean (S_UI, "track-autoarm"));
      adw_action_row_add_suffix (switch_row, GTK_WIDGET (s));
      gtk_widget_set_valign (GTK_WIDGET (s), GTK_ALIGN_CENTER);
      g_signal_connect (
        s, "notify::active", G_CALLBACK (on_auto_arm_switch_changed), self);
      adw_preferences_group_add (pgroup, GTK_WIDGET (switch_row));
    }
  }

  gtk_widget_set_size_request (GTK_WIDGET (self), 400, -1);
}
