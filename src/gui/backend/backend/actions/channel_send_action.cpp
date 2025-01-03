// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_identifier.h"
#include "gui/backend/backend/actions/channel_send_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/dsp/router.h"
#include "gui/dsp/tracklist.h"
#include "utils/rt_thread_id.h"

using namespace zrythm::gui::actions;

ChannelSendAction::ChannelSendAction (QObject * parent)
    : QObject (parent), UndoableAction (UndoableAction::Type::ChannelSend)
{
}

ChannelSendAction::ChannelSendAction (
  Type                           type,
  const ChannelSend             &send,
  const Port *                   port,
  const StereoPorts *            stereo,
  float                          amount,
  const PortConnectionsManager * port_connections_mgr)
    : UndoableAction (UndoableAction::Type::ChannelSend),
      send_before_ (send.clone_unique ()), amount_ (amount),
      send_action_type_ (type)
{
  if (port != nullptr)
    {
      midi_id_ = port->id_->clone_unique ();
    }

  if (stereo != nullptr)
    {
      l_id_ = stereo->get_l ().id_->clone_unique ();
      r_id_ = stereo->get_r ().id_->clone_unique ();
    }

  if (port_connections_mgr)
    {
      port_connections_before_ = port_connections_mgr->clone_unique ();
    }
}

void
ChannelSendAction::init_after_cloning (const ChannelSendAction &other)
{
  UndoableAction::copy_members_from (other);
  send_before_ = other.send_before_->clone_unique ();
  amount_ = other.amount_;
  if (other.l_id_ != nullptr)
    {
      l_id_ = other.l_id_->clone_unique ();
    }
  if (other.r_id_ != nullptr)
    {
      r_id_ = other.r_id_->clone_unique ();
    }
  if (other.midi_id_ != nullptr)
    {
      midi_id_ = other.midi_id_->clone_unique ();
    }
  send_action_type_ = other.send_action_type_;
}

bool
ChannelSendAction::connect_or_disconnect (bool connect, bool do_it)
{
  /* get the actual channel send from the project */
  auto send = send_before_->find_in_project ();

  send->disconnect (false);

  if (do_it)
    {
      if (connect)
        {
          auto track = send->get_track ();
          switch (track->out_signal_type_)
            {
            case PortType::Event:
              {
                const auto port_var = PROJECT->find_port_by_id (*midi_id_);
                z_return_val_if_fail (
                  port_var.has_value ()
                    && std::holds_alternative<MidiPort *> (port_var.value ()),
                  false);
                auto * port = std::get<MidiPort *> (port_var.value ());
                send->connect_midi (*port, false, true);
              }
              break;
            case PortType::Audio:
              {
                const auto l_var = PROJECT->find_port_by_id (*l_id_);
                z_return_val_if_fail (
                  l_var.has_value ()
                    && std::holds_alternative<AudioPort *> (l_var.value ()),
                  false);
                auto *     l = std::get<AudioPort *> (l_var.value ());
                const auto r_var = PROJECT->find_port_by_id (*r_id_);
                z_return_val_if_fail (
                  r_var.has_value ()
                    && std::holds_alternative<AudioPort *> (r_var.value ()),
                  false);
                auto * r = std::get<AudioPort *> (r_var.value ());
                send->connect_stereo (
                  nullptr, l, r, send_action_type_ == Type::ConnectSidechain,
                  false, true);
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

  return true;
}

void
ChannelSendAction::perform_impl ()
{
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
      send->set_amount (amount_);
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
}

void
ChannelSendAction::undo_impl ()
{
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
      send->set_amount (send_before_->amount_->control_);
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

      TRACKLIST->validate ();
    }

  /* EVENTS_PUSH (EventType::ET_CHANNEL_SEND_CHANGED, send); */
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
