// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/session/graph_dispatcher.h"
#include "gui/backend/backend/actions/channel_send_action.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/tracklist.h"
#include "utils/rt_thread_id.h"

namespace zrythm::gui::actions
{

ChannelSendAction::ChannelSendAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::ChannelSend)
{
}

ChannelSendAction::ChannelSendAction (
  Type               type,
  const ChannelSend &send,
  const dsp::Port *  port,
  std::optional<std::pair<const dsp::AudioPort &, const dsp::AudioPort &>> stereo,
  float                          amount,
  const PortConnectionsManager * port_connections_mgr)
    : UndoableAction (UndoableAction::Type::ChannelSend),
      send_before_ (
        utils::clone_unique (
          send,
          utils::ObjectCloneType::Snapshot,
          PROJECT->get_track_registry (),
          PROJECT->get_port_registry (),
          PROJECT->get_param_registry ())),
      amount_ (amount), send_action_type_ (type)
{
  if (port != nullptr)
    {
      midi_id_ = port->get_uuid ();
    }

  if (stereo.has_value ())
    {
      l_id_ = stereo->first.get_uuid ();
      r_id_ = stereo->second.get_uuid ();
    }

  if (port_connections_mgr != nullptr)
    {
      port_connections_before_ = utils::clone_unique (*port_connections_mgr);
    }
}

void
init_from (
  ChannelSendAction       &obj,
  const ChannelSendAction &other,
  utils::ObjectCloneType   clone_type)
{
  init_from (
    static_cast<UndoableAction &> (obj),
    static_cast<const UndoableAction &> (other), clone_type);
  obj.send_before_ = utils::clone_unique (
    *other.send_before_, clone_type, other.send_before_->track_registry_,
    other.send_before_->port_registry_, other.send_before_->param_registry_);
  obj.amount_ = other.amount_;
  obj.l_id_ = other.l_id_;
  obj.r_id_ = other.r_id_;
  obj.midi_id_ = other.midi_id_;
  obj.send_action_type_ = other.send_action_type_;
}

bool
ChannelSendAction::connect_or_disconnect (bool connect, bool do_it)
{
// TODO
#if 0
  /* get the actual channel send from the project */
  auto send = send_before_->find_in_project ();

  send->disconnect (false);

  if (do_it)
    {
      if (connect)
        {
          auto track = send->get_track ();
          switch (track->get_output_signal_type ())
            {
            case PortType::Event:
              {
                const auto port_var = PROJECT->find_port_by_id (*midi_id_);
                z_return_val_if_fail (
                  port_var.has_value ()
                    && std::holds_alternative<dsp::MidiPort *> (
                      port_var.value ()),
                  false);
                auto * port = std::get<dsp::MidiPort *> (port_var.value ());
                send->connect_midi (*port, false, true);
              }
              break;
            case PortType::Audio:
              {
                const auto l_var = PROJECT->find_port_by_id (*l_id_);
                z_return_val_if_fail (
                  l_var.has_value ()
                    && std::holds_alternative<dsp::AudioPort *> (l_var.value ()),
                  false);
                auto *     l = std::get<dsp::AudioPort *> (l_var.value ());
                const auto r_var = PROJECT->find_port_by_id (*r_id_);
                z_return_val_if_fail (
                  r_var.has_value ()
                    && std::holds_alternative<dsp::AudioPort *> (r_var.value ()),
                  false);
                auto * r = std::get<dsp::AudioPort *> (r_var.value ());
                send->connect_stereo (
                  *l, *r, send_action_type_ == Type::ConnectSidechain, false,
                  true);
              }
              break;
            default:
              break;
            }
        }
    }
  /* else if not doing */
  else
    {
      /* copy the values - connections will be reverted later when resetting the
       * connections manager */
      send->copy_values_from (*send_before_);
    }
#endif
  return true;
}

void
ChannelSendAction::perform_impl ()
{
  // TODO
#if 0
  /* get the actual channel send from the project */
  ChannelSend * send = send_before_->find_in_project ();

  bool need_restore_and_recalc = false;

  bool successful = false;
  switch (send_action_type_)
    {
    case Type::ConnectMidi:
    case Type::ConnectStereo:
    case Type::ChangePorts:
    case Type::ConnectSidechain:
      successful = connect_or_disconnect (true, true);
      need_restore_and_recalc = true;
      break;
    case Type::Disconnect:
      successful = connect_or_disconnect (false, true);
      need_restore_and_recalc = true;
      break;
    case Type::ChangeAmount:
      successful = true;
      send->set_amount_in_amplitude (amount_);
      break;
    default:
      break;
    }

  if (!successful)
    {
      throw ZrythmException ("Channel send operation failed");
    }

  if (need_restore_and_recalc)
    {
      save_or_load_port_connections (true);

      ROUTER->recalc_graph (false);
    }

  /* EVENTS_PUSH (EventType::ET_CHANNEL_SEND_CHANGED, send); */
#endif
}

void
ChannelSendAction::undo_impl ()
{
// TODO
#if 0
  /* get the actual channel send from the project */
  ChannelSend * send = send_before_->find_in_project ();

  bool need_restore_and_recalc = false;

  bool successful = false;
  switch (send_action_type_)
    {
    case Type::ConnectMidi:
    case Type::ConnectStereo:
    case Type::ConnectSidechain:
      successful = connect_or_disconnect (false, true);
      need_restore_and_recalc = true;
      break;
    case Type::ChangePorts:
    case Type::Disconnect:
      successful = connect_or_disconnect (true, false);
      need_restore_and_recalc = true;
      break;
    case Type::ChangeAmount:
      send->set_amount_in_amplitude (send_before_->get_current_amount_value ());
      successful = true;
      break;
    default:
      break;
    }

  if (!successful)
    {
      throw ZrythmException ("Failed to undo channel send action");
    }

  if (need_restore_and_recalc)
    {
      save_or_load_port_connections (false);

      ROUTER->recalc_graph (false);
    }

  /* EVENTS_PUSH (EventType::ET_CHANNEL_SEND_CHANGED, send); */
#endif
}

QString
ChannelSendAction::to_string () const
{
  switch (send_action_type_)
    {
    case Type::ConnectSidechain:
      return QObject::tr ("Connect sidechain");
    case Type::ConnectStereo:
      return QObject::tr ("Connect stereo");
    case Type::ConnectMidi:
      return QObject::tr ("Connect MIDI");
    case Type::Disconnect:
      return QObject::tr ("Disconnect");
    case Type::ChangeAmount:
      return QObject::tr ("Change amount");
    case Type::ChangePorts:
      return QObject::tr ("Change ports");
    }
  return QObject::tr ("Channel send connection");
}
}
