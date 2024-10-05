// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/settings/settings.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/channel_slot.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/plugin_strip_expander.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/channel.h"
#include "common/dsp/track.h"
#include "common/utils/gtk.h"
#include "common/utils/rt_thread_id.h"

#define PLUGIN_STRIP_EXPANDER_WIDGET_TYPE \
  (plugin_strip_expander_widget_get_type ())
G_DEFINE_TYPE (
  PluginStripExpanderWidget,
  plugin_strip_expander_widget,
  EXPANDER_BOX_WIDGET_TYPE)

/**
 * Queues a redraw of the given slot.
 */
void
plugin_strip_expander_widget_redraw_slot (
  PluginStripExpanderWidget * self,
  int                         slot)
{
  switch (self->slot_type)
    {
    case PluginSlotType::Insert:
    case PluginSlotType::MidiFx:
      gtk_widget_queue_draw (GTK_WIDGET (self->slots[slot]));
      break;
    default:
      break;
    }
}

/**
 * Refreshes each field.
 */
void
plugin_strip_expander_widget_refresh (PluginStripExpanderWidget * self)
{
  z_return_if_fail (self->track);
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      switch (self->slot_type)
        {
        case PluginSlotType::Insert:
          {
            auto ch = self->track->get_channel ();
            z_return_if_fail (ch);
            gtk_widget_queue_draw (GTK_WIDGET (self->slots[i]));
          }
          break;
        case PluginSlotType::MidiFx:
          {
            auto ch = self->track->get_channel ();
            z_return_if_fail (ch);
            gtk_widget_queue_draw (GTK_WIDGET (self->slots[i]));
          }
          break;
        default:
          break;
        }
    }
}

static void
on_reveal_changed (
  ExpanderBoxWidget *         expander_box,
  bool                        revealed,
  PluginStripExpanderWidget * self)
{
  if (self->position == PluginStripExpanderPosition::PSE_POSITION_CHANNEL)
    {
      auto ch = self->track->get_channel ();
      if (self->slot_type == PluginSlotType::Insert)
        {
          g_settings_set_boolean (S_UI_MIXER, "inserts-expanded", revealed);
          EVENTS_PUSH (
            EventType::ET_MIXER_CHANNEL_INSERTS_EXPANDED_CHANGED, ch.get ());
        }
      else if (self->slot_type == PluginSlotType::MidiFx)
        {
          g_settings_set_boolean (S_UI_MIXER, "midi-fx-expanded", revealed);
          EVENTS_PUSH (
            EventType::ET_MIXER_CHANNEL_MIDI_FX_EXPANDED_CHANGED, ch.get ());
        }
    }
}

/**
 * Sets up the PluginStripExpanderWidget.
 */
void
plugin_strip_expander_widget_setup (
  PluginStripExpanderWidget * self,
  PluginSlotType              slot_type,
  PluginStripExpanderPosition position,
  ChannelTrack *              track)
{
  z_return_if_fail (track);

  /* set name and icon */
  char fullstr[200];
  bool is_midi = false;
  switch (slot_type)
    {
    case PluginSlotType::Insert:
      strcpy (fullstr, _ ("Inserts"));
      is_midi = track && track->out_signal_type_ == PortType::Event;
      break;
    case PluginSlotType::MidiFx:
      strcpy (fullstr, "MIDI FX");
      is_midi = true;
      break;
    default:
      z_return_if_reached ();
      break;
    }
  expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), fullstr);

  if (is_midi)
    {
      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self), "midi-insert");
    }
  else
    {
      expander_box_widget_set_icon_name (
        Z_EXPANDER_BOX_WIDGET (self), "audio-insert");
    }

  if (
    track != self->track || slot_type != self->slot_type
    || position != self->position)
    {
      /* remove children */
      z_gtk_widget_destroy_all_children (GTK_WIDGET (self->box));

      auto ch = track->get_channel ();
      z_return_if_fail (ch);
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          GtkBox * strip_box =
            GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
          gtk_widget_set_name (
            GTK_WIDGET (strip_box), "plugin-strip-expander-strip-box");
          self->strip_boxes[i] = strip_box;

          switch (slot_type)
            {
            case PluginSlotType::Insert:
            case PluginSlotType::MidiFx:
              {
                ChannelSlotWidget * csw = channel_slot_widget_new (
                  i, track, slot_type,
                  position == PluginStripExpanderPosition::PSE_POSITION_CHANNEL);
                self->slots[i] = csw;
                gtk_box_append (strip_box, GTK_WIDGET (csw));
              }
              break;
            default:
              z_warn_if_reached ();
              break;
            }

          gtk_box_append (self->box, GTK_WIDGET (strip_box));
        }
    }

  self->track = track;
  self->slot_type = slot_type;
  self->position = position;

  switch (position)
    {
    case PluginStripExpanderPosition::PSE_POSITION_INSPECTOR:
      gtk_widget_set_size_request (GTK_WIDGET (self->scroll), -1, 124);
      break;
    case PluginStripExpanderPosition::PSE_POSITION_CHANNEL:
      gtk_widget_set_size_request (GTK_WIDGET (self->scroll), -1, -1);
      if (
        slot_type == PluginSlotType::Insert
        || slot_type == PluginSlotType::MidiFx)
        {
          expander_box_widget_set_reveal_callback (
            Z_EXPANDER_BOX_WIDGET (self),
            (ExpanderBoxRevealFunc) on_reveal_changed, self);
        }
      break;
    }

  plugin_strip_expander_widget_refresh (self);
}

static void
plugin_strip_expander_widget_class_init (PluginStripExpanderWidgetClass * klass)
{
}

static void
plugin_strip_expander_widget_init (PluginStripExpanderWidget * self)
{
  self->scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_widget_set_name (
    GTK_WIDGET (self->scroll), "plugin-strip-expander-scroll");
  gtk_widget_set_vexpand (GTK_WIDGET (self->scroll), 1);
  /*gtk_scrolled_window_set_shadow_type (*/
  /*self->scroll, GTK_SHADOW_ETCHED_IN);*/

  self->viewport = GTK_VIEWPORT (gtk_viewport_new (nullptr, nullptr));
  gtk_viewport_set_scroll_to_focus (self->viewport, false);
  gtk_widget_set_name (
    GTK_WIDGET (self->viewport), "plugin-strip-expander-viewport");
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (self->scroll), GTK_WIDGET (self->viewport));

  self->box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_name (GTK_WIDGET (self->box), "plugin-strip-expander-box");
  gtk_viewport_set_child (GTK_VIEWPORT (self->viewport), GTK_WIDGET (self->box));

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->scroll));
}
