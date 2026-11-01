// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <vector>

#include "plugins/plugin_descriptor.h"

#include <QAbstractListModel>
#include <QReadWriteLock>
#include <QStringList>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

namespace zrythm::plugins::discovery
{

/**
 * @brief Canonical list model of the descriptors of all scanned plugins.
 *
 * The model also owns scan bookkeeping (blacklisted identifiers and scan
 * timestamps). Descriptor-changing methods must be called on the model's
 * thread; scan workers clone descriptors through the read-only accessors and
 * apply their results on the model thread.
 */
class PluginCatalog : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    QStringList availableFormats READ availableFormats NOTIFY
      availableFormatsChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit PluginCatalog (QObject * parent = nullptr);
  ~PluginCatalog () override;

  PluginCatalog (const PluginCatalog &) = delete;
  PluginCatalog &operator= (const PluginCatalog &) = delete;
  PluginCatalog (PluginCatalog &&) = delete;
  PluginCatalog &operator= (PluginCatalog &&) = delete;

  enum PluginDescriptorRole
  {
    DescriptorRole = Qt::UserRole + 1,
    DescriptorNameRole,
  };

  /** Result of checking whether an identifier needs scanning. */
  enum class ScanState
  {
    /** The identifier has descriptors and none require rescanning. */
    UpToDate,
    /** The identifier has no descriptors or a changed plugin file. */
    NeedsScan,
    /** A previous scan of the identifier failed. */
    Blacklisted,
  };

  QHash<int, QByteArray> roleNames () const override;
  QModelIndex
  index (int row, int column, const QModelIndex &parent = QModelIndex ())
    const override;
  QModelIndex parent (const QModelIndex &child) const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  int columnCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  /** Returns the distinct plugin formats present in the catalog, sorted. */
  [[nodiscard]] QStringList availableFormats () const;

  /**
   * @brief Returns the scan state of an identifier for @p protocol.
   *
   * @param needs_rescanning Returns whether a stored descriptor's plugin file
   * changed since it was scanned.
   */
  [[nodiscard]] ScanState scan_state (
    Protocol::ProtocolType   protocol,
    const utils::Utf8String &identifier,
    const std::function<bool (const PluginDescriptor &)> &needs_rescanning) const;

  /** Returns clones of every descriptor an identifier produced. */
  [[nodiscard]] std::vector<std::unique_ptr<PluginDescriptor>>
  descriptors_for_identifier (
    Protocol::ProtocolType   protocol,
    const utils::Utf8String &identifier) const;

  /**
   * @brief Adds or replaces a single descriptor.
   *
   * @return Whether a new descriptor identity was added.
   */
  bool add_descriptor (std::unique_ptr<PluginDescriptor> descriptor);

  /**
   * @brief Replaces all descriptors scanned from @p identifier for @p protocol.
   *
   * An empty @p descriptors value removes stale descriptors for that protocol
   * and identifier without blacklisting it.
   *
   * @return An error when @p descriptors contains two descriptors with the
   * same identity; the catalog is left unchanged.
   */
  std::expected<void, utils::Utf8String> replace_results_for_scan (
    Protocol::ProtocolType                         protocol,
    const utils::Utf8String                       &identifier,
    std::vector<std::unique_ptr<PluginDescriptor>> descriptors);

  /** Removes the descriptor with the same identity as @p descriptor. */
  void remove_descriptor (const PluginDescriptor &descriptor);

  /** Removes all descriptors and blacklist entries. */
  void clear ();

  [[nodiscard]] int descriptor_count () const;

  /** Returns clones of all descriptors in row order. */
  [[nodiscard]] std::vector<std::unique_ptr<PluginDescriptor>>
  descriptors () const;

  [[nodiscard]] bool is_blacklisted (
    Protocol::ProtocolType   protocol,
    const utils::Utf8String &identifier) const;
  void blacklist_identifier (
    Protocol::ProtocolType   protocol,
    const utils::Utf8String &identifier);
  void remove_from_blacklist (
    Protocol::ProtocolType   protocol,
    const utils::Utf8String &identifier);
  void clear_blacklist ();

  /** Serializes the catalog in the known-plugins JSON format. */
  [[nodiscard]] nlohmann::json serialize () const;

  /**
   * @brief Replaces the catalog contents with @p json.
   *
   * @throws ZrythmException when the JSON does not match the known-plugins
   * schema or contains invalid descriptor data.
   */
  void deserialize (const nlohmann::json &json);

Q_SIGNALS:
  void availableFormatsChanged ();

private:
  static QString descriptor_key (const PluginDescriptor &descriptor);

  /** Returns whether the persistent metadata of two descriptors is equal. */
  static bool
  same_metadata (const PluginDescriptor &a, const PluginDescriptor &b);

  /** Copies the scan timestamps from @p fresh into @p stored. */
  static void refresh_scan_timestamps (
    PluginDescriptor       &stored,
    const PluginDescriptor &fresh);

  struct BlacklistEntry
  {
    Protocol::ProtocolType protocol_;
    utils::Utf8String      identifier_;

    bool operator== (const BlacklistEntry &) const = default;
  };

  /** Updates the format list from the descriptors. The write lock must be held.
   */
  void update_available_formats ();

private:
  mutable QReadWriteLock lock_;

  std::vector<std::unique_ptr<PluginDescriptor>> descriptors_;
  std::vector<BlacklistEntry>                    blacklisted_identifiers_;
  QStringList                                    available_formats_;
};

} // namespace zrythm::plugins::discovery
