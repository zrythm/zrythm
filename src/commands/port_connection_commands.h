// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>

#include "commands/command.h"
#include "dsp/port.h"
#include "structure/project/port_connections_manager.h"

namespace zrythm::commands
{

/**
 * @brief Command that adds a user connection between two ports.
 */
class AddPortConnectionCommand : public Command
{
public:
  AddPortConnectionCommand (
    structure::project::PortConnectionsManager &manager,
    const dsp::PortUuid                         src,
    const dsp::PortUuid                         dest,
    dsp::Port                                  &destination_port);

  void undo () override;
  void redo () override;

private:
  structure::project::PortConnectionsManager &manager_;
  dsp::PortUuid                               src_;
  dsp::PortUuid                               dest_;
  dsp::Port                                  &destination_port_;
  /** Whether the destination joins the graph only while connected. */
  bool attachment_follows_connections_ = false;
};

/**
 * @brief Command that removes a connection between two ports, restoring the
 * full connection state on undo.
 */
class RemovePortConnectionCommand : public Command
{
public:
  RemovePortConnectionCommand (
    structure::project::PortConnectionsManager &manager,
    const dsp::PortUuid                         src,
    const dsp::PortUuid                         dest,
    dsp::Port                                  &destination_port);

  void undo () override;
  void redo () override;

private:
  structure::project::PortConnectionsManager &manager_;
  dsp::PortUuid                               src_;
  dsp::PortUuid                               dest_;
  dsp::Port                                  &destination_port_;
  /** Whether the destination joins the graph only while connected. */
  bool                        attachment_follows_connections_ = false;
  float                       multiplier_ = 1.0f;
  bool                        locked_ = false;
  bool                        enabled_ = true;
  bool                        bipolar_ = false;
  dsp::AudioBusChannelRouting routing_;
};

/**
 * @brief Command that changes the multiplier or enabled state of an
 * existing user connection.
 *
 * The state before the change is captured on construction; undo restores
 * it. The connection's locked flag is passed through unchanged.
 */
class UpdatePortConnectionCommand : public Command
{
public:
  /**
   * QUndoCommand id for merge participation: Qt only calls mergeWith()
   * between commands sharing a non-negative id.
   */
  static constexpr auto kMergeId = 6451641;

  /**
   * @param merges Whether consecutive commands on the same connection
   * merge into a single undo step (used for drag gestures). Commands merge
   * only within one second of the previous redo.
   */
  UpdatePortConnectionCommand (
    structure::project::PortConnectionsManager &manager,
    const dsp::PortUuid                         src,
    const dsp::PortUuid                         dest,
    float                                       multiplier,
    bool                                        enabled,
    bool                                        merges);

  int  id () const override { return kMergeId; }
  bool mergeWith (const QUndoCommand * other) override;
  void undo () override;
  void redo () override;

private:
  structure::project::PortConnectionsManager        &manager_;
  dsp::PortUuid                                      src_;
  dsp::PortUuid                                      dest_;
  float                                              multiplier_ = 1.0f;
  bool                                               enabled_ = true;
  float                                              before_multiplier_ = 1.0f;
  bool                                               before_enabled_ = true;
  bool                                               merges_ = false;
  std::chrono::time_point<std::chrono::steady_clock> last_redo_timestamp_{};
};

} // namespace zrythm::commands
