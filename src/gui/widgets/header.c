/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include "gui/accel.h"
#include "gui/widgets/header.h"
#include "gui/widgets/help_toolbar.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/project_toolbar.h"
#include "gui/widgets/rotated_label.h"
#include "gui/widgets/snap_box.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/toolbox.h"
#include "gui/widgets/view_toolbar.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  HeaderWidget, header_widget, GTK_TYPE_BOX)

void
header_widget_refresh (
  HeaderWidget * self)
{
  /* TODO move to main window */
#if 0
  g_debug (
    "refreshing header widget: "
    "has pending warnings: %d",
    self->log_has_pending_warnings);
  GtkButton * btn = self->log_viewer;
  z_gtk_button_set_emblem (
    btn,
    self->log_has_pending_warnings ?
      "media-record" : NULL);
#endif
}

void
header_widget_setup (
  HeaderWidget * self,
  const char * title)
{
  home_toolbar_widget_setup (self->home_toolbar);
}

static void
header_widget_init (
  HeaderWidget * self)
{
  g_type_ensure (HOME_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (TOOLBOX_WIDGET_TYPE);
  g_type_ensure (SNAP_BOX_WIDGET_TYPE);
  g_type_ensure (SNAP_GRID_WIDGET_TYPE);
  g_type_ensure (HELP_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (VIEW_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (PROJECT_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (LIVE_WAVEFORM_WIDGET_TYPE);
  g_type_ensure (MIDI_ACTIVITY_BAR_WIDGET_TYPE);
  g_type_ensure (ROTATED_LABEL_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkStyleContext *context;
  context =
    gtk_widget_get_style_context (
      GTK_WIDGET (self));
  gtk_style_context_add_class (
    context, "header");

  live_waveform_widget_setup_engine (
    self->live_waveform);
  midi_activity_bar_widget_setup_engine (
    self->midi_activity);
  midi_activity_bar_widget_set_animation (
    self->midi_activity,
    MAB_ANIMATION_FLASH);

  /* setup the rotated label */
  rotated_label_widget_setup (
    self->midi_in_rotated_lbl, -90);
  GtkLabel * lbl =
    rotated_label_widget_get_label (
    self->midi_in_rotated_lbl);
  gtk_widget_add_css_class (
    GTK_WIDGET (lbl), "small-vertical-lbl");
  const char * midi_in_txt_translated =
    /* TRANSLATORS: either leave this
     * untranslated or make sure the translated text
     * is max 7 characters */
    _("MIDI in");
  const char * midi_in_txt = midi_in_txt_translated;
  if (g_utf8_strlen (midi_in_txt, -1) > 8)
    midi_in_txt = "MIDI in";
  rotated_label_widget_set_markup (
    self->midi_in_rotated_lbl, midi_in_txt);

  /* set tooltips */
  char about_tooltip[500];
  sprintf (
    about_tooltip, _("About %s"), PROGRAM_NAME);
}

static void
header_widget_class_init (
  HeaderWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);

  resources_set_class_template (
    klass, "header.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, HeaderWidget, x)

  BIND_CHILD (stack);
  BIND_CHILD (home_toolbar);
  BIND_CHILD (project_toolbar);
  BIND_CHILD (view_toolbar);
  BIND_CHILD (help_toolbar);
  BIND_CHILD (live_waveform);
  BIND_CHILD (midi_activity);
  BIND_CHILD (midi_in_rotated_lbl);

#undef BIND_CHILD
}
