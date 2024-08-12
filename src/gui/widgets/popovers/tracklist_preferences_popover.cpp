// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstring>

#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/widgets/popovers/tracklist_preferences_popover.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"
#include "libadwaita_wrapper.h"

G_DEFINE_TYPE (
  TracklistPreferencesPopoverWidget,
  tracklist_preferences_popover_widget,
  GTK_TYPE_POPOVER)

TracklistPreferencesPopoverWidget *
tracklist_preferences_popover_widget_new (void)
{
  TracklistPreferencesPopoverWidget * self =
    Z_TRACKLIST_PREFERENCES_POPOVER_WIDGET (
      g_object_new (TRACKLIST_PREFERENCES_POPOVER_WIDGET_TYPE, nullptr));

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
      AdwSwitchRow * switch_row = ADW_SWITCH_ROW (adw_switch_row_new ());
      adw_preferences_row_set_title (
        ADW_PREFERENCES_ROW (switch_row), _ ("Auto-arm for Recording"));
      adw_action_row_set_subtitle (
        ADW_ACTION_ROW (switch_row),
        _ ("Arm tracks for recording when clicked/selected."));
      g_settings_bind (
        S_UI, "track-autoarm", switch_row, "active", G_SETTINGS_BIND_DEFAULT);
      adw_preferences_group_add (pgroup, GTK_WIDGET (switch_row));
    }

    {
      AdwSwitchRow * switch_row = ADW_SWITCH_ROW (adw_switch_row_new ());
      adw_preferences_row_set_title (
        ADW_PREFERENCES_ROW (switch_row), _ ("Auto-select"));
      adw_action_row_set_subtitle (
        ADW_ACTION_ROW (switch_row),
        _ ("Select tracks when their regions are clicked/selected."));
      g_settings_bind (
        S_UI, "track-autoselect", switch_row, "active", G_SETTINGS_BIND_DEFAULT);
      adw_preferences_group_add (pgroup, GTK_WIDGET (switch_row));
    }
  }

  gtk_widget_set_size_request (GTK_WIDGET (self), 400, -1);
}
