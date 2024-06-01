// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/channel_send_action.h"
#include "dsp/channel_send.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_send_selector.h"
#include "gui/widgets/item_factory.h"
#include "plugins/plugin_identifier.h"
#include "project.h"
#include "utils/error.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSendSelectorWidget,
  channel_send_selector_widget,
  GTK_TYPE_POPOVER)

static Track *
get_track_from_target (ChannelSendTarget * target)
{
  if (target->type == ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_NONE)
    return NULL;

  return TRACKLIST->tracks[target->track_pos];
}

static StereoPorts *
get_sidechain_from_target (ChannelSendTarget * target)
{
  if (
    target->type
    != ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_PLUGIN_SIDECHAIN)
    {
      return NULL;
    }

  Plugin * pl = plugin_find (&target->pl_id);
  g_return_val_if_fail (pl, NULL);
  Port l = plugin_get_port_in_group (pl, target->port_group, true)->clone ();
  Port r = plugin_get_port_in_group (pl, target->port_group, false)->clone ();
  return new StereoPorts (std::move (l), std::move (r));
}

/**
 * Called when row is double clicked.
 */
static void
on_row_activated (
  GtkListView *               list_view,
  guint                       position,
  ChannelSendSelectorWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self), false);
}

static void
on_selection_changed (
  GtkSelectionModel *         selection_model,
  guint                       position,
  guint                       n_items,
  ChannelSendSelectorWidget * self)
{
  GObject * gobj =
    G_OBJECT (gtk_single_selection_get_selected_item (self->view_model));
  if (!gobj)
    return;

  /* get wrapped object */
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gobj);
  ChannelSendTarget * target = (ChannelSendTarget *) wrapped_obj->obj;

  ChannelSend * send = self->send_widget->send;
  bool          is_empty = channel_send_is_empty (send);

  Track *          src_track = channel_send_get_track (send);
  Track *          dest_track = NULL;
  StereoPorts *    dest_sidechain = NULL;
  PortConnection * conn = NULL;
  ;
  switch (target->type)
    {
    case ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_NONE:
      if (channel_send_is_enabled (send))
        {
          GError * err = NULL;
          bool     ret = channel_send_action_perform_disconnect (
            self->send_widget->send, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to disconnect send"));
            }
        }
      break;
    case ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_TRACK:
      dest_track = get_track_from_target (target);
      switch (src_track->out_signal_type)
        {
        case PortType::Event:
          if (
            port_connections_manager_get_sources_or_dests (
              PORT_CONNECTIONS_MGR, NULL, &send->midi_out->id_, false)
            == 1)
            {
              conn = port_connections_manager_get_source_or_dest (
                PORT_CONNECTIONS_MGR, &send->midi_out->id_, false);
            }
          if (
            is_empty
            || (conn && !conn->dest_id->is_equal (dest_track->processor->midi_in->id_)))
            {
              GError * err = NULL;
              bool     ret = channel_send_action_perform_connect_midi (
                send, dest_track->processor->midi_in, &err);
              if (!ret)
                {
                  HANDLE_ERROR (err, "%s", _ ("Failed to connect send"));
                }
            }
          break;
        case PortType::Audio:
          if (
            port_connections_manager_get_sources_or_dests (
              PORT_CONNECTIONS_MGR, NULL, &send->stereo_out->get_l ().id_, false)
            == 1)
            {
              conn = port_connections_manager_get_source_or_dest (
                PORT_CONNECTIONS_MGR, &send->stereo_out->get_l ().id_, false);
            }
          if (is_empty || (conn && !conn->dest_id->is_equal(dest_track->processor->stereo_in->get_l().id_)))
            {
              GError * err = NULL;
              bool     ret = channel_send_action_perform_connect_audio (
                send, dest_track->processor->stereo_in, &err);
              if (!ret)
                {
                  HANDLE_ERROR (err, "%s", _ ("Failed to connect send"));
                }
            }
          break;
        default:
          break;
        }
      break;
    case ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_PLUGIN_SIDECHAIN:
      {
        dest_sidechain = get_sidechain_from_target (target);
        if (
          port_connections_manager_get_sources_or_dests (
            PORT_CONNECTIONS_MGR, NULL, &send->stereo_out->get_l ().id_, false)
          == 1)
          {
            conn = port_connections_manager_get_source_or_dest (
              PORT_CONNECTIONS_MGR, &send->stereo_out->get_l ().id_, false);
          }
        if (dest_sidechain && (is_empty || !send->is_sidechain || (conn && !conn->dest_id->is_equal(dest_sidechain->get_l().id_))))
          {
            GError * err = NULL;
            bool     ret = channel_send_action_perform_connect_sidechain (
              send, dest_sidechain, &err);
            if (!ret)
              {
                HANDLE_ERROR (err, "%s", _ ("Failed to connect send"));
              }
          }
        object_zero_and_free (dest_sidechain);
      }
      break;
    }

  gtk_widget_queue_draw (GTK_WIDGET (self->send_widget));
}

static void
setup_view (ChannelSendSelectorWidget * self)
{
  /* icon, name, pointer to data */
  GListStore * list_store =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  {
    ChannelSendTarget * target = object_new (ChannelSendTarget);
    target->type = ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_NONE;
    WrappedObjectWithChangeSignal * wobj =
      wrapped_object_with_change_signal_new_with_free_func (
        target, WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET,
        (ObjectFreeFunc) channel_send_target_free);
    g_list_store_append (list_store, wobj);
  }

  unsigned int select_idx = 0;
  unsigned int count = 1;

  /* setup tracks */
  ChannelSend * send = self->send_widget->send;
  Track *       track = channel_send_get_track (send);
  for (size_t i = 0; i < TRACKLIST->tracks.size (); ++i)
    {
      Track * target_track = TRACKLIST->tracks[i];

      g_debug ("target %s", target_track->name);

      /* skip tracks with non-matching signal types */
      if (
        target_track == track
        || track->out_signal_type != target_track->in_signal_type
        || !track_type_is_fx (target_track->type))
        continue;
      g_message ("adding %s", target_track->name);

      /* create target */
      ChannelSendTarget * target = object_new (ChannelSendTarget);
      target = object_new (ChannelSendTarget);
      target->type = ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_TRACK;
      target->track_pos = i;

      /* add it to list */
      WrappedObjectWithChangeSignal * wobj =
        wrapped_object_with_change_signal_new_with_free_func (
          target, WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET,
          (ObjectFreeFunc) channel_send_target_free);
      g_list_store_append (list_store, wobj);

      if (
        channel_send_is_enabled (send)
        && target_track == channel_send_get_target_track (send, NULL))
        {
          select_idx = count;
        }
      count++;
    }

  /* setup plugin sidechain inputs */
  g_debug ("setting up sidechains");
  for (auto target_track : TRACKLIST->tracks)
    {
      if (target_track == track || !track_type_has_channel (target_track->type))
        {
          continue;
        }

      Channel * ch = target_track->channel;

      Plugin * plugins[300];
      int      num_plugins = ch->get_plugins (plugins);

      for (int j = 0; j < num_plugins; j++)
        {
          Plugin * pl = plugins[j];
          g_debug ("plugin %s", pl->setting->descr->name);

          for (int k = 0; k < pl->num_in_ports; k++)
            {
              Port * port = pl->in_ports[k];
              g_debug ("port %s", port->id_.label_.c_str ());

              if (
                !(ENUM_BITSET_TEST (
                  PortIdentifier::Flags, port->id_.flags_,
                  PortIdentifier::Flags::SIDECHAIN))
                || port->id_.type_ != PortType::Audio
                || port->id_.port_group_.empty ()
                || !(ENUM_BITSET_TEST (
                  PortIdentifier::Flags, port->id_.flags_,
                  PortIdentifier::Flags::STEREO_L)))
                {
                  continue;
                }

              /* find corresponding port in the same port group (e.g., if this
               * is left, find right and vice versa) */
              Port * other_channel = plugin_get_port_in_same_group (pl, port);
              if (!other_channel)
                {
                  continue;
                }
              g_debug ("other channel %s", other_channel->id_.label_.c_str ());

              Port *l = NULL, *r = NULL;
              if (
                ENUM_BITSET_TEST (
                  PortIdentifier::Flags, port->id_.flags_,
                  PortIdentifier::Flags::STEREO_L)
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags, other_channel->id_.flags_,
                  PortIdentifier::Flags::STEREO_R))
                {
                  l = port;
                  r = other_channel;
                }
              if (!l || !r)
                {
                  continue;
                }

              /* create target */
              ChannelSendTarget * target = object_new (ChannelSendTarget);
              target->type =
                ChannelSendTargetType::CHANNEL_SEND_TARGET_TYPE_PLUGIN_SIDECHAIN;
              target->track_pos = target_track->pos;
              target->pl_id = pl->id;
              target->port_group = g_strdup (l->id_.port_group_.c_str ());

              /* add it to list */
              WrappedObjectWithChangeSignal * wobj =
                wrapped_object_with_change_signal_new_with_free_func (
                  target,
                  WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET,
                  (ObjectFreeFunc) channel_send_target_free);
              g_list_store_append (list_store, wobj);

              if (channel_send_is_target_sidechain (send))
                {
                  StereoPorts * sp = channel_send_get_target_sidechain (send);
                  if (&sp->get_l () == l && &sp->get_r () == r)
                    {
                      select_idx = count;
                    }
                  object_zero_and_free (sp);
                }
              count++;
            }
        }
    }

  self->view_model = gtk_single_selection_new (G_LIST_MODEL (list_store));
  gtk_list_view_set_model (self->view, GTK_SELECTION_MODEL (self->view_model));
  self->item_factory =
    item_factory_new (ItemFactoryType::ITEM_FACTORY_ICON_AND_TEXT, false, NULL);
  gtk_list_view_set_factory (self->view, self->item_factory->list_item_factory);

  gtk_selection_model_select_item (
    GTK_SELECTION_MODEL (self->view_model), select_idx, true);

  g_signal_connect (
    G_OBJECT (self->view_model), "selection-changed",
    G_CALLBACK (on_selection_changed), self);
}

void
channel_send_selector_widget_setup (ChannelSendSelectorWidget * self)
{
  setup_view (self);
}

ChannelSendSelectorWidget *
channel_send_selector_widget_new (ChannelSendWidget * send)
{
  ChannelSendSelectorWidget * self = static_cast<ChannelSendSelectorWidget *> (
    g_object_new (CHANNEL_SEND_SELECTOR_WIDGET_TYPE, NULL));
  self->send_widget = send;

  return self;
}

static void
channel_send_selector_widget_class_init (ChannelSendSelectorWidgetClass * klass)
{
}

static void
channel_send_selector_widget_init (ChannelSendSelectorWidget * self)
{
  /* create box */
  self->vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_popover_set_child (GTK_POPOVER (self), GTK_WIDGET (self->vbox));

  /* add scroll */
  GtkWidget * scroll = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (
    GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_propagate_natural_height (
    GTK_SCROLLED_WINDOW (scroll), true);
  gtk_scrolled_window_set_max_content_height (GTK_SCROLLED_WINDOW (scroll), 240);
  gtk_box_append (GTK_BOX (self->vbox), scroll);

  /* add view */
  self->view = GTK_LIST_VIEW (gtk_list_view_new (NULL, NULL));
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll), GTK_WIDGET (self->view));
  g_signal_connect (
    G_OBJECT (self->view), "activate", G_CALLBACK (on_row_activated), self);
}
