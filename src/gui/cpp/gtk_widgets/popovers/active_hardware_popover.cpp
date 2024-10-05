// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/channel.h"
#include "common/dsp/ext_port.h"
#include "common/dsp/hardware_processor.h"
#include "common/utils/gtk.h"
#include "common/utils/resources.h"
#include "common/utils/string.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/active_hardware_mb.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/popovers/active_hardware_popover.h"

G_DEFINE_TYPE (
  ActiveHardwarePopoverWidget,
  active_hardware_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_closed (ActiveHardwarePopoverWidget * self, gpointer user_data)
{
  active_hardware_mb_widget_refresh (self->owner);
}

static void
get_controllers (
  ActiveHardwarePopoverWidget * self,
  std::vector<std::string>     &controllers)
{
  if (!PROJECT)
    {
      return;
    }

  auto &processor =
    self->owner->input
      ? AUDIO_ENGINE->hw_in_processor_
      : AUDIO_ENGINE->hw_out_processor_;

  /* force a rescan just in case */
  processor->rescan_ext_ports (processor.get ());
  auto &ports =
    self->owner->is_midi ? processor->ext_midi_ports_ : processor->ext_audio_ports_;
  for (auto &port : ports)
    {
      controllers.push_back (port->get_id ());
    }
}

/**
 * Finds checkbutton with given label.
 */
static GtkWidget *
find_checkbutton (ActiveHardwarePopoverWidget * self, const char * label)
{
  for (
    GtkWidget * child =
      gtk_widget_get_first_child (GTK_WIDGET (self->controllers_box));
    child != NULL; child = gtk_widget_get_next_sibling (child))
    {
      if (!GTK_IS_CHECK_BUTTON (child))
        continue;

      GtkCheckButton * chkbtn = GTK_CHECK_BUTTON (child);
      if (string_is_equal (gtk_check_button_get_label (chkbtn), label))
        {
          return GTK_WIDGET (chkbtn);
        }
    }

  return NULL;
}

static void
setup (ActiveHardwarePopoverWidget * self)
{
  /* remove pre-existing controllers */
  z_gtk_widget_destroy_all_children (GTK_WIDGET (self->controllers_box));

  /* scan controllers and add them */
  std::vector<std::string> controllers;
  get_controllers (self, controllers);
  for (auto &controller : controllers)
    {
      GtkWidget * chkbtn = gtk_check_button_new_with_label (controller.c_str ());
      gtk_box_append (self->controllers_box, chkbtn);
    }

  /* fetch saved controllers and tick them if they
   * exist */
  gchar ** saved_controllers =
    g_settings_get_strv (self->owner->settings, self->owner->key);
  char * tmp;
  size_t i = 0;
  while ((tmp = saved_controllers[i]) != nullptr)
    {
      /* find checkbutton matching saved
       * controller */
      GtkWidget * chkbtn = find_checkbutton (self, tmp);

      if (chkbtn)
        {
          /* tick it */
          gtk_check_button_set_active (GTK_CHECK_BUTTON (chkbtn), 1);
        }

      i++;
    }
}

static void
on_rescan (GtkButton * btn, ActiveHardwarePopoverWidget * self)
{
  setup (self);
}

ActiveHardwarePopoverWidget *
active_hardware_popover_widget_new (ActiveHardwareMbWidget * owner)
{
  ActiveHardwarePopoverWidget * self = Z_ACTIVE_HARDWARE_POPOVER_WIDGET (
    g_object_new (ACTIVE_HARDWARE_POPOVER_WIDGET_TYPE, nullptr));

  self->owner = owner;

  setup (self);

  return self;
}

static void
active_hardware_popover_widget_class_init (
  ActiveHardwarePopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "active_hardware_popover.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ActiveHardwarePopoverWidget, x)

  BIND_CHILD (rescan);
  BIND_CHILD (controllers_box);

#undef BIND_CHILD

  gtk_widget_class_bind_template_callback (klass, on_closed);
}

static void
active_hardware_popover_widget_init (ActiveHardwarePopoverWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    G_OBJECT (self->rescan), "clicked", G_CALLBACK (on_rescan), self);
}
