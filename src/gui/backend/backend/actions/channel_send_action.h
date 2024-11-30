// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_CHANNEL_SEND_ACTION_H__
#define __UNDO_CHANNEL_SEND_ACTION_H__

#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/dsp/channel_send.h"
#include "gui/dsp/port_connections_manager.h"

namespace zrythm::gui::actions
{

/**
 * Action for channel send changes.
 */
class ChannelSendAction
    : public QObject,
      public UndoableAction,
      public ICloneable<ChannelSendAction>,
      public zrythm::utils::serialization::ISerializable<ChannelSendAction>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (ChannelSendAction)

public:
  enum class Type
  {
    ConnectStereo,
    ConnectMidi,
    ConnectSidechain,
    ChangeAmount,
    ChangePorts,
    Disconnect,
  };

  using PortType = zrythm::dsp::PortType;
  using PortIdentifier = dsp::PortIdentifier;

public:
  ChannelSendAction (QObject * parent = nullptr);

  /**
   * Creates a new action.
   *
   * @param port MIDI port, if connecting MIDI.
   * @param stereo Stereo ports, if connecting audio.
   * @param port_connections_mgr Port connections
   *   manager at the start of the action, if needed.
   */
  ChannelSendAction (
    Type                           type,
    const ChannelSend             &send,
    const Port *                   port,
    const StereoPorts *            stereo,
    float                          amount,
    const PortConnectionsManager * port_connections_mgr);

  QString to_string () const override;

  void init_after_cloning (const ChannelSendAction &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void init_loaded_impl () override { }
  void perform_impl () override;
  void undo_impl () override;

  bool connect_or_disconnect (bool connect, bool do_it);

public:
  std::unique_ptr<ChannelSend> send_before_;

  float amount_ = 0.f;

  /** Target port identifiers. */
  std::unique_ptr<PortIdentifier> l_id_;
  std::unique_ptr<PortIdentifier> r_id_;
  std::unique_ptr<PortIdentifier> midi_id_;

  /** Action type. */
  Type send_action_type_ = Type ();
};

class ChannelSendDisconnectAction final : public ChannelSendAction
{
public:
  ChannelSendDisconnectAction (
    const ChannelSend            &send,
    const PortConnectionsManager &port_connections_mgr)
      : ChannelSendAction (Type::Disconnect, send, nullptr, nullptr, 0.f, &port_connections_mgr)
  {
  }
};

class ChannelSendConnectMidiAction final : public ChannelSendAction
{
public:
  ChannelSendConnectMidiAction (
    const ChannelSend            &send,
    const Port                   &midi,
    const PortConnectionsManager &port_connections_mgr)
      : ChannelSendAction (Type::ConnectMidi, send, &midi, nullptr, 0.f, &port_connections_mgr)
  {
  }
};

class ChannelSendConnectStereoAction final : public ChannelSendAction
{
public:
  ChannelSendConnectStereoAction (
    const ChannelSend            &send,
    const StereoPorts            &stereo,
    const PortConnectionsManager &port_connections_mgr)
      : ChannelSendAction (Type::ConnectStereo, send, nullptr, &stereo, 0.f, &port_connections_mgr)
  {
  }
};

class ChannelSendConnectSidechainAction final : public ChannelSendAction
{
public:
  ChannelSendConnectSidechainAction (
    const ChannelSend            &send,
    const StereoPorts            &sidechain,
    const PortConnectionsManager &port_connections_mgr)
      : ChannelSendAction (
          Type::ConnectSidechain,
          send,
          nullptr,
          &sidechain,
          0.f,
          &port_connections_mgr)
  {
  }
};

class ChannelSendChangeAmountAction final : public ChannelSendAction
{
public:
  ChannelSendChangeAmountAction (const ChannelSend &send, float amount)
      : ChannelSendAction (Type::ChangeAmount, send, nullptr, nullptr, amount, nullptr)
  {
  }
};

}; // namespace zrythm::gui::actions

#endif
