// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/preroll_count_selector.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PrerollCountSelectorWidget,
  preroll_count_selector_widget,
  GTK_TYPE_BOX)

static void
block_all_handlers (PrerollCountSelectorWidget * self, bool block)
{
  if (block)
    {
      g_signal_handler_block (self->none_toggle, self->none_toggle_id);
      g_signal_handler_block (self->one_bar_toggle, self->one_bar_toggle_id);
      g_signal_handler_block (self->two_bars_toggle, self->two_bars_toggle_id);
      g_signal_handler_block (self->four_bars_toggle, self->four_bars_toggle_id);
    }
  else
    {
      g_signal_handler_unblock (self->none_toggle, self->none_toggle_id);
      g_signal_handler_unblock (self->one_bar_toggle, self->one_bar_toggle_id);
      g_signal_handler_unblock (self->two_bars_toggle, self->two_bars_toggle_id);
      g_signal_handler_unblock (
        self->four_bars_toggle, self->four_bars_toggle_id);
    }
}

static void
on_btn_toggled (GtkToggleButton * btn, PrerollCountSelectorWidget * self)
{
  block_all_handlers (self, true);
  gtk_toggle_button_set_active (self->none_toggle, false);
  gtk_toggle_button_set_active (self->one_bar_toggle, false);
  gtk_toggle_button_set_active (self->two_bars_toggle, false);
  gtk_toggle_button_set_active (self->four_bars_toggle, false);

  PrerollCountBars new_bars = PrerollCountBars::PREROLL_COUNT_BARS_NONE;
  if (btn == self->none_toggle)
    {
      gtk_toggle_button_set_active (self->none_toggle, true);
      new_bars = PrerollCountBars::PREROLL_COUNT_BARS_NONE;
    }
  else if (btn == self->one_bar_toggle)
    {
      gtk_toggle_button_set_active (self->one_bar_toggle, true);
      new_bars = PrerollCountBars::PREROLL_COUNT_BARS_ONE;
    }
  else if (btn == self->two_bars_toggle)
    {
      gtk_toggle_button_set_active (self->two_bars_toggle, true);
      new_bars = PrerollCountBars::PREROLL_COUNT_BARS_TWO;
    }
  else if (btn == self->four_bars_toggle)
    {
      gtk_toggle_button_set_active (self->four_bars_toggle, true);
      new_bars = PrerollCountBars::PREROLL_COUNT_BARS_FOUR;
    }

  if (
    self->type
    == PrerollCountSelectorType::PREROLL_COUNT_SELECTOR_METRONOME_COUNTIN)
    {
      g_settings_set_enum (
        S_TRANSPORT, "metronome-countin", ENUM_VALUE_TO_INT (new_bars));
    }
  else if (
    self->type
    == PrerollCountSelectorType::PREROLL_COUNT_SELECTOR_RECORD_PREROLL)
    {
      g_settings_set_enum (
        S_TRANSPORT, "recording-preroll", ENUM_VALUE_TO_INT (new_bars));
    }
  block_all_handlers (self, false);
}

/**
 * Creates a PrerollCountSelectorWidget.
 */
PrerollCountSelectorWidget *
preroll_count_selector_widget_new (PrerollCountSelectorType type)
{
  PrerollCountSelectorWidget * self = static_cast<
    PrerollCountSelectorWidget *> (g_object_new (
    PREROLL_COUNT_SELECTOR_WIDGET_TYPE, "orientation",
    GTK_ORIENTATION_HORIZONTAL, "visible", true, nullptr));

  self->type = type;

#define CREATE(x, lbl) \
  self->x##_toggle = \
    GTK_TOGGLE_BUTTON (gtk_toggle_button_new_with_label (lbl)); \
  gtk_widget_set_visible (GTK_WIDGET (self->x##_toggle), true); \
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->x##_toggle)); \
  self->x##_toggle_id = g_signal_connect ( \
    G_OBJECT (self->x##_toggle), "toggled", G_CALLBACK (on_btn_toggled), self)

  CREATE (none, _ (preroll_count_bars_str[0]));
  CREATE (one_bar, _ (preroll_count_bars_str[1]));
  CREATE (two_bars, _ (preroll_count_bars_str[2]));
  CREATE (four_bars, _ (preroll_count_bars_str[3]));

  PrerollCountBars preroll_type = PrerollCountBars::PREROLL_COUNT_BARS_NONE;
  switch (type)
    {
    case PrerollCountSelectorType::PREROLL_COUNT_SELECTOR_METRONOME_COUNTIN:
      preroll_type = ENUM_INT_TO_VALUE (
        PrerollCountBars,
        g_settings_get_enum (S_TRANSPORT, "metronome-countin"));
      break;
    case PrerollCountSelectorType::PREROLL_COUNT_SELECTOR_RECORD_PREROLL:
      preroll_type = ENUM_INT_TO_VALUE (
        PrerollCountBars,
        g_settings_get_enum (S_TRANSPORT, "recording-preroll"));
      break;
    }

  switch (preroll_type)
    {
    case PrerollCountBars::PREROLL_COUNT_BARS_NONE:
      gtk_toggle_button_set_active (self->none_toggle, true);
      break;
    case PrerollCountBars::PREROLL_COUNT_BARS_ONE:
      gtk_toggle_button_set_active (self->one_bar_toggle, true);
      break;
    case PrerollCountBars::PREROLL_COUNT_BARS_TWO:
      gtk_toggle_button_set_active (self->two_bars_toggle, true);
      break;
    case PrerollCountBars::PREROLL_COUNT_BARS_FOUR:
      gtk_toggle_button_set_active (self->four_bars_toggle, true);
      break;
    default:
      z_return_val_if_reached (nullptr);
    }

  return self;
}

static void
preroll_count_selector_widget_init (PrerollCountSelectorWidget * self)
{
  gtk_widget_add_css_class (GTK_WIDGET (self), "linked");
  gtk_widget_add_css_class (GTK_WIDGET (self), "bounce-step-selector");
}

static void
preroll_count_selector_widget_class_init (
  PrerollCountSelectorWidgetClass * _klass)
{
}
