// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/exporter.h"
#include "gui/widgets/bounce_step_selector.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"
#include <zix/ring.h>

G_DEFINE_TYPE (BounceStepSelectorWidget, bounce_step_selector_widget, GTK_TYPE_BOX)

static void
block_all_handlers (BounceStepSelectorWidget * self, bool block)
{
  if (block)
    {
      g_signal_handler_block (
        self->before_inserts_toggle, self->before_inserts_toggle_id);
      g_signal_handler_block (self->pre_fader_toggle, self->pre_fader_toggle_id);
      g_signal_handler_block (
        self->post_fader_toggle, self->post_fader_toggle_id);
    }
  else
    {
      g_signal_handler_unblock (
        self->before_inserts_toggle, self->before_inserts_toggle_id);
      g_signal_handler_unblock (
        self->pre_fader_toggle, self->pre_fader_toggle_id);
      g_signal_handler_unblock (
        self->post_fader_toggle, self->post_fader_toggle_id);
    }
}

static void
on_before_inserts_toggled (GtkToggleButton * btn, BounceStepSelectorWidget * self)
{
  g_settings_set_enum (
    S_UI, "bounce-step",
    ENUM_VALUE_TO_INT (BounceStep::BOUNCE_STEP_BEFORE_INSERTS));

  block_all_handlers (self, true);
  gtk_toggle_button_set_active (self->before_inserts_toggle, true);
  gtk_toggle_button_set_active (self->pre_fader_toggle, false);
  gtk_toggle_button_set_active (self->post_fader_toggle, false);
  block_all_handlers (self, false);
}

static void
on_pre_fader_toggled (GtkToggleButton * btn, BounceStepSelectorWidget * self)
{
  g_settings_set_enum (
    S_UI, "bounce-step", ENUM_VALUE_TO_INT (BounceStep::BOUNCE_STEP_PRE_FADER));

  block_all_handlers (self, true);
  gtk_toggle_button_set_active (self->before_inserts_toggle, true);
  gtk_toggle_button_set_active (self->pre_fader_toggle, true);
  gtk_toggle_button_set_active (self->post_fader_toggle, false);
  block_all_handlers (self, false);
}

static void
on_post_fader_toggled (GtkToggleButton * btn, BounceStepSelectorWidget * self)
{
  g_settings_set_enum (
    S_UI, "bounce-step", ENUM_VALUE_TO_INT (BounceStep::BOUNCE_STEP_POST_FADER));

  block_all_handlers (self, true);
  gtk_toggle_button_set_active (self->before_inserts_toggle, true);
  gtk_toggle_button_set_active (self->pre_fader_toggle, true);
  gtk_toggle_button_set_active (self->post_fader_toggle, true);
  block_all_handlers (self, false);
}

/**
 * Creates a BounceStepSelectorWidget.
 */
BounceStepSelectorWidget *
bounce_step_selector_widget_new (void)
{
  BounceStepSelectorWidget * self = static_cast<
    BounceStepSelectorWidget *> (g_object_new (
    BOUNCE_STEP_SELECTOR_WIDGET_TYPE, "orientation", GTK_ORIENTATION_VERTICAL,
    NULL));

  gtk_widget_set_visible (GTK_WIDGET (self), true);

#define CREATE(x, n, icon) \
  self->x##_toggle = z_gtk_toggle_button_new_with_icon_and_text ( \
    icon, _ (bounce_step_str[n]), false, GTK_ORIENTATION_HORIZONTAL, 4); \
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->x##_toggle)); \
  self->x##_toggle_id = g_signal_connect ( \
    G_OBJECT (self->x##_toggle), "toggled", G_CALLBACK (on_##x##_toggled), \
    self)

  CREATE (before_inserts, 0, "audio-insert");
  CREATE (pre_fader, 1, "effect");
  CREATE (post_fader, 2, "fader");

  BounceStep step = (BounceStep) g_settings_get_enum (S_UI, "bounce-step");
  switch (step)
    {
    case BounceStep::BOUNCE_STEP_BEFORE_INSERTS:
      gtk_toggle_button_set_active (self->before_inserts_toggle, true);
      break;
    case BounceStep::BOUNCE_STEP_PRE_FADER:
      gtk_toggle_button_set_active (self->pre_fader_toggle, true);
      break;
    case BounceStep::BOUNCE_STEP_POST_FADER:
      gtk_toggle_button_set_active (self->post_fader_toggle, true);
      break;
    }

  return self;
}

static void
bounce_step_selector_widget_init (BounceStepSelectorWidget * self)
{
  gtk_widget_add_css_class (GTK_WIDGET (self), "linked");
  gtk_widget_add_css_class (GTK_WIDGET (self), "bounce-step-selector");
#if 0
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), _("Bounce step selector"));
  gdk_rgba_parse (&self->text_fg, "white");
#endif
}

static void
bounce_step_selector_widget_class_init (BounceStepSelectorWidgetClass * _klass)
{
#if 0
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "bounce-step-selector");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
#endif
}
