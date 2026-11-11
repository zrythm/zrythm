// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>

#include "dsp/port.h"
#include "structure/tracks/channel_send.h"
#include "utils/iobject_registry.h"
#include "utils/registry_utils.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::tracks
{

/**
 * @brief A channel's list of sends, exposed to QML as a list model.
 *
 * Sends are registry objects referenced by UUID; the group owns no
 * objects itself. A removed send stays registered while undo commands
 * hold a reference, so undo can reinsert it with its identity intact.
 */
class ChannelSendGroup : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChannelSendGroup (
    utils::IObjectRegistry &registry,
    dsp::PortType           signal_type,
    QObject *               parent = nullptr);
  Q_DISABLE_COPY_MOVE (ChannelSendGroup)
  ~ChannelSendGroup () override;

  // ============================================================================
  // QML Interface
  // ============================================================================

  enum class Roles
  {
    SendRole = Qt::UserRole + 1,
  };
  Q_ENUM (Roles)

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // ============================================================================

  /**
   * @brief Creates a send, registers it and appends it to the group.
   *
   * @param is_prefader Whether the send taps the channel before its fader.
   * @return Reference to the send.
   */
  ChannelSendUuidReference append_send (bool is_prefader);

  /**
   * @brief Inserts a send back into the group at @p index (-1 appends).
   *
   * The index is clamped to [-1, count]: -1 appends and a larger index
   * inserts at the end.
   *
   * @return Reference to the reinserted send.
   */
  ChannelSendUuidReference
  insert_send (const ChannelSendUuidReference &send, int index = -1);

  /**
   * @brief Removes a send from the group.
   *
   * The send stays registered; callers that need it to survive must hold
   * their own reference.
   *
   * @throws std::invalid_argument if no send with the UUID is in the group.
   */
  void remove_send (const ChannelSend::Uuid &id);

  ChannelSend * send_at_index (int index) const;

  std::span<const ChannelSendUuidReference> get_sends () const
  {
    return sends_;
  }

  utils::IObjectRegistry &registry () const { return registry_; }

  dsp::PortType signal_type () const { return signal_type_; }

private:
  static constexpr auto kSendsKey = "sends"sv;
  friend void           to_json (nlohmann::json &j, const ChannelSendGroup &g);
  friend void from_json (const nlohmann::json &j, ChannelSendGroup &g);

private:
  utils::IObjectRegistry &registry_;

  /** Signal type shared by all sends in the group (the channel's output
   * type). */
  dsp::PortType signal_type_;

  std::vector<ChannelSendUuidReference> sends_;
};

} // namespace zrythm::structure::tracks
