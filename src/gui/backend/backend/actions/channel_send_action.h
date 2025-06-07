// SPDX-FileCopyrightText: © 2020-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_connections_manager.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "structure/tracks/channel_send.h"

namespace zrythm::gui::actions
{

/**
 * Action for channel send changes.
 */
class ChannelSendAction : public QObject, public UndoableAction
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (ChannelSendAction)

  using ChannelSend = structure::tracks::ChannelSend;

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
    Type                                                           type,
    const ChannelSend                                             &send,
    const Port *                                                   port,
    std::optional<std::pair<const AudioPort &, const AudioPort &>> stereo,
    float                                                          amount,
    const dsp::PortConnectionsManager * port_connections_mgr);

  QString to_string () const override;

  friend void init_from (
    ChannelSendAction       &obj,
    const ChannelSendAction &other,
    utils::ObjectCloneType   clone_type);

private:
  void init_loaded_impl () override { }
  void perform_impl () override;
  void undo_impl () override;

  bool connect_or_disconnect (bool connect, bool do_it);

public:
  std::unique_ptr<ChannelSend> send_before_;

  float amount_ = 0.f;

  /** Target port identifiers. */
  std::optional<PortIdentifier::PortUuid> l_id_;
  std::optional<PortIdentifier::PortUuid> r_id_;
  std::optional<PortIdentifier::PortUuid> midi_id_;

  /** Action type. */
  Type send_action_type_ = Type ();
};

class ChannelSendDisconnectAction final : public ChannelSendAction
{
public:
  ChannelSendDisconnectAction (
    const ChannelSend                 &send,
    const dsp::PortConnectionsManager &port_connections_mgr)
      : ChannelSendAction (Type::Disconnect, send, nullptr, std::nullopt, 0.f, &port_connections_mgr)
  {
  }
};

class ChannelSendConnectMidiAction final : public ChannelSendAction
{
public:
  ChannelSendConnectMidiAction (
    const ChannelSend                 &send,
    const Port                        &midi,
    const dsp::PortConnectionsManager &port_connections_mgr)
      : ChannelSendAction (Type::ConnectMidi, send, &midi, std::nullopt, 0.f, &port_connections_mgr)
  {
  }
};

class ChannelSendConnectStereoAction final : public ChannelSendAction
{
public:
  ChannelSendConnectStereoAction (
    const ChannelSend                              &send,
    std::pair<const AudioPort &, const AudioPort &> stereo,
    const dsp::PortConnectionsManager              &port_connections_mgr)
      : ChannelSendAction (Type::ConnectStereo, send, nullptr, stereo, 0.f, &port_connections_mgr)
  {
  }
};

class ChannelSendConnectSidechainAction final : public ChannelSendAction
{
public:
  ChannelSendConnectSidechainAction (
    const ChannelSend                              &send,
    std::pair<const AudioPort &, const AudioPort &> sidechain,
    const dsp::PortConnectionsManager              &port_connections_mgr)
      : ChannelSendAction (
          Type::ConnectSidechain,
          send,
          nullptr,
          sidechain,
          0.f,
          &port_connections_mgr)
  {
  }
};

class ChannelSendChangeAmountAction final : public ChannelSendAction
{
public:
  ChannelSendChangeAmountAction (const ChannelSend &send, float amount)
      : ChannelSendAction (Type::ChangeAmount, send, nullptr, std::nullopt, amount, nullptr)
  {
  }
};

}; // namespace zrythm::gui::actions
