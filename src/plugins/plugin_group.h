// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "dsp/fader.h"
#include "plugins/plugin_all.h"
#include "utils/qt.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::plugins
{
/**
 * @brief A flexible container for plugins and nested plugin groups.
 *
 * PluginGroup provides a unified architecture for implementing racks, chains,
 * and layers. It supports both serial and parallel processing modes, recursive
 * nesting, and type-safe signal routing for Audio, MIDI, and Instrument
 * processing.
 *
 * Key features:
 * - Universal container for plugins and nested groups
 * - Serial processing: chain elements sequentially
 * - Parallel processing: process elements separately and sum outputs
 * - Recursive nesting: groups can contain other groups
 * - Type safety: separate Audio, MIDI, and Instrument signal types
 * - Built-in fader for level control and muting/soloing
 *
 * Common use cases:
 * - Simple plugin wrapper with fader control
 * - Effect chains (serial processing)
 * - Layered instruments (parallel processing)
 * - Complex nested rack structures
 *
 * Integration:
 * - Used in channels for MIDI FX, Instrument, and Audio FX processing
 * - Exposes QML model interface for UI integration
 */
class PluginGroup : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum class ProcessingTypeHint : std::uint8_t
  {
    /**
     * @brief Process each element in sequence, connecting the output to the
     * next element's input, and the last output to the fader.
     *
     * This can be used to mimic a processing rack.
     */
    Serial,

    /**
     * @brief Process all elements separately and sum their outputs to the fader.
     *
     * This can be used to mimic layers inside racks.
     */
    Parallel,

    /**
     * @brief Processing is completely up to the user of this class.
     */
    Custom,
  };
  Q_ENUM (ProcessingTypeHint)

  enum class DeviceGroupType : std::uint8_t
  {
    Audio,
    MIDI,
    Instrument,
    CV,
  };
  Q_ENUM (DeviceGroupType)

  PluginGroup (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    plugins::PluginRegistry                      &plugin_registry,
    DeviceGroupType                               type,
    ProcessingTypeHint                            processing_type,
    QObject *                                     parent = nullptr);
  Z_DISABLE_COPY_MOVE (PluginGroup)
  ~PluginGroup () override;

  // ============================================================================
  // QML Interface
  // ============================================================================

  QString       name () const { return name_; }
  void          setName (const QString &name);
  Q_SIGNAL void nameChanged (const QString &name);

  enum DeviceGroupListModelRoles
  {
    DeviceGroupPtrRole = Qt::UserRole + 1,
  };
  Q_ENUM (DeviceGroupListModelRoles)

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
  // ============================================================================

  // Note: plugin add/remove API does not concern itself with automation tracks
  // or graph rebuilding.
  // When plugins are                                   added or removed,
  // automation tracks should be generated/moved accordingly and the DSP graph
  // should be regenerated.

  void insert_plugin (plugins::PluginUuidReference plugin_ref, int index = -1);
  void append_plugin (plugins::PluginUuidReference plugin_ref)
  {
    insert_plugin (std::move (plugin_ref), -1);
  }
  plugins::PluginUuidReference
  remove_plugin (const plugins::Plugin::Uuid &plugin_id);

  QVariant element_at_idx (size_t idx) const;

  /**
   * @brief Returns all plugins in the group (scanning recursively).
   *
   * @param pls Vector to add plugins to.
   */
  void get_plugins (std::vector<plugins::PluginPtrVariant> &plugins) const;

private:
  static constexpr auto kDeviceGroupsKey = "deviceGroups"sv;
  friend void           to_json (nlohmann::json &j, const PluginGroup &l);
  friend void           from_json (const nlohmann::json &j, PluginGroup &l);

private:
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies_;
  plugins::PluginRegistry                      &plugin_registry_;
  const ProcessingTypeHint                      processing_type_;
  const DeviceGroupType                         type_;
  utils::QObjectUniquePtr<dsp::Fader>           fader_;
  QString                                       name_;

  struct DeviceGroupImpl;
  std::unique_ptr<DeviceGroupImpl> pimpl_;
};
}
