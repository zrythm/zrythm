/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

#include "audio/engine.h"
#include "audio/engine_jack.h"
#include "audio/transport.h"
#include "gui/widgets/cpu.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/transport_controls.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (TopBarWidget,
               top_bar_widget,
               GTK_TYPE_BOX)

#ifdef HAVE_JACK
static void
on_jack_transport_type_changed (
  GtkCheckMenuItem * menuitem,
  TopBarWidget *     self)
{
  if (menuitem == self->timebase_master_check &&
        gtk_check_menu_item_get_active (menuitem))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER);
      gtk_widget_set_visible (
        self->master_img, 1);
      gtk_widget_set_visible (
        self->client_img, 0);
    }
  else if (menuitem ==
             self->transport_client_check &&
           gtk_check_menu_item_get_active (
             menuitem))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AUDIO_ENGINE_JACK_TRANSPORT_CLIENT);
      gtk_widget_set_visible (
        self->master_img, 0);
      gtk_widget_set_visible (
        self->client_img, 1);
    }
  else if (menuitem ==
             self->no_jack_transport_check &&
           gtk_check_menu_item_get_active (
              menuitem))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AUDIO_ENGINE_NO_JACK_TRANSPORT);
      gtk_widget_set_visible (
        self->master_img, 0);
      gtk_widget_set_visible (
        self->client_img, 0);
    }
}
#endif

static void
on_transport_playhead_right_click (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  TopBarWidget *      self)
{
  if (n_press != 1)
    return;

  GtkWidget *menu, *menuitem, *menuitem2;

  menu = gtk_menu_new();

  /* jack transport related */
#ifdef HAVE_JACK
  menuitem =
    gtk_radio_menu_item_new_with_mnemonic (
      NULL,
      _("JACK Timebase master"));
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    AUDIO_ENGINE->transport_type ==
      AUDIO_ENGINE_JACK_TIMEBASE_MASTER);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_jack_transport_type_changed),
    self);
  /*gtk_actionable_set_action_name (*/
    /*GTK_ACTIONABLE (menuitem),*/
    /*"set-timebase-master");*/
  self->timebase_master_check =
    GTK_CHECK_MENU_ITEM (menuitem);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);
  menuitem2 =
    gtk_radio_menu_item_new_with_mnemonic (
      NULL,
      _("JACK Transport client"));
  gtk_radio_menu_item_join_group (
    GTK_RADIO_MENU_ITEM (menuitem2),
    GTK_RADIO_MENU_ITEM (menuitem));
  menuitem = menuitem2;
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    AUDIO_ENGINE->transport_type ==
      AUDIO_ENGINE_JACK_TRANSPORT_CLIENT);
  self->transport_client_check =
    GTK_CHECK_MENU_ITEM (menuitem);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_jack_transport_type_changed),
    self);
  /*gtk_actionable_set_action_name (*/
    /*GTK_ACTIONABLE (menuitem),*/
    /*"set-transport-client");*/
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);
  menuitem2 =
    gtk_radio_menu_item_new_with_mnemonic (
      NULL,
      _("Unlink JACK Transport"));
  gtk_radio_menu_item_join_group (
    GTK_RADIO_MENU_ITEM (menuitem2),
    GTK_RADIO_MENU_ITEM (menuitem));
  menuitem = menuitem2;
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    AUDIO_ENGINE->transport_type ==
      AUDIO_ENGINE_NO_JACK_TRANSPORT);
  self->no_jack_transport_check =
    GTK_CHECK_MENU_ITEM (menuitem);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_jack_transport_type_changed),
    self);
  /*gtk_actionable_set_action_name (*/
    /*GTK_ACTIONABLE (menuitem),*/
    /*"unlink-jack-transport");*/
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);
#endif

  menuitem =
    gtk_separator_menu_item_new ();

  /* display format related */
  /*gtk_menu_shell_append (*/
    /*GTK_MENU_SHELL (menu), menuitem);*/
  /*menuitem =*/
    /*gtk_radio_menu_item_new_with_mnemonic (*/
      /*NULL,*/
      /*_("Show seconds"));*/

  /*g_signal_connect(menuitem, "activate",*/
                   /*(GCallback) view_popup_menu_onDoSomething, treeview);*/

  /*gtk_menu_shell_append (*/
    /*GTK_MENU_SHELL (menu), menuitem);*/

  gtk_widget_show_all (menu);

  gtk_menu_popup_at_pointer (
    GTK_MENU (menu), NULL);
}

void
top_bar_widget_refresh (TopBarWidget * self)
{
  if (self->digital_transport)
    gtk_widget_destroy (
      GTK_WIDGET (self->digital_transport));
  self->digital_transport =
    digital_meter_widget_new_for_position (
      TRANSPORT,
      NULL,
      transport_get_playhead_pos,
      transport_set_playhead_pos,
      NULL,
      _("playhead"));
  self->playhead_overlay =
    GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->playhead_overlay), 1);
  gtk_container_add (
    GTK_CONTAINER (self->playhead_overlay),
    GTK_WIDGET (self->digital_transport));

#ifdef HAVE_JACK
  int size = 8;
  GdkPixbuf * pixbuf;
  GtkWidget * img;

  pixbuf =
    gtk_icon_theme_load_icon (
      gtk_icon_theme_get_default (),
      "jack_transport_client",
      size, 0, NULL);
  img =
    gtk_image_new_from_pixbuf (pixbuf);
  gtk_widget_set_halign (
    img, GTK_ALIGN_END);
  gtk_widget_set_valign (
    img, GTK_ALIGN_START);
  gtk_widget_set_visible (img, 1);
  gtk_widget_set_tooltip_text (
    img,
    _("JACK Transport client"));
  gtk_overlay_add_overlay (
    self->playhead_overlay, img);
  self->client_img = img;
  pixbuf =
    gtk_icon_theme_load_icon (
      gtk_icon_theme_get_default (),
      "jack_timebase_master",
      size, 0, NULL);
  img =
    gtk_image_new_from_pixbuf (pixbuf);
  gtk_widget_set_halign (
    img, GTK_ALIGN_END);
  gtk_widget_set_valign (
    img, GTK_ALIGN_START);
  gtk_widget_set_margin_end (
    img, size + 2);
  gtk_widget_set_visible (img, 1);
  gtk_widget_set_tooltip_text (
    img,
    _("JACK Timebase master"));
  gtk_overlay_add_overlay (
    self->playhead_overlay, img);
  self->master_img = img;
  if (AUDIO_ENGINE->transport_type ==
        AUDIO_ENGINE_JACK_TRANSPORT_CLIENT)
    {
      gtk_widget_set_visible (
        self->master_img, 0);
      gtk_widget_set_visible (
        self->client_img, 1);
    }
  else if (AUDIO_ENGINE->transport_type ==
             AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
    {
      gtk_widget_set_visible (
        self->master_img, 1);
      gtk_widget_set_visible (
        self->client_img, 0);
    }
#endif

  GtkGestureMultiPress * right_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (
      right_mp),
      GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_mp), "pressed",
    G_CALLBACK (on_transport_playhead_right_click),
    self);

  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->playhead_overlay));
}

static void
top_bar_widget_class_init (TopBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "top_bar.ui");

  gtk_widget_class_set_css_name (klass,
                                 "top-bar");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    TopBarWidget, \
    child)

  BIND_CHILD (digital_meters);
  BIND_CHILD (transport_controls);
  BIND_CHILD (live_waveform);
  BIND_CHILD (midi_activity);
  BIND_CHILD (cpu_load);

#undef BIND_CHILD
}

static void
top_bar_widget_init (TopBarWidget * self)
{
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      DIGITAL_METER_WIDGET_TYPE, NULL)));
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      TRANSPORT_CONTROLS_WIDGET_TYPE, NULL)));
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      CPU_WIDGET_TYPE, NULL)));

  gtk_widget_init_template (GTK_WIDGET (self));

  /* setup digital meters */
  self->digital_bpm =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_BPM,
      NULL,
      NULL, "bpm");
  self->digital_timesig =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_TIMESIG,
      NULL,
      NULL, _("time sig."));
  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->digital_bpm));
  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->digital_timesig));
  gtk_widget_show_all (
    GTK_WIDGET (self->digital_meters));
  gtk_widget_show_all (GTK_WIDGET (self));

  live_waveform_widget_setup_engine (
    self->live_waveform);
  midi_activity_bar_widget_setup_engine (
    self->midi_activity);
  midi_activity_bar_widget_set_animation (
    self->midi_activity,
    MAB_ANIMATION_FLASH);
  cpu_widget_setup (
    self->cpu_load);
}
