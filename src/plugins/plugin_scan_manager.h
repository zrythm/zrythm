// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 * ---
 *
 * This file is part of the JUCE framework.
 * Copyright (c) Raw Material Software Limited
 *
 * JUCE is an open source framework subject to commercial or open source
 * licensing.
 *
 * By downloading, installing, or using the JUCE framework, or combining the
 * JUCE framework with any other source code, object code, content or any other
 * copyrightable work, you agree to the terms of the JUCE End User Licence
 * Agreement, and all incorporated terms including the JUCE Privacy Policy and
 * the JUCE Website Terms of Service, as applicable, which will bind you. If you
 * do not agree to the terms of these agreements, we will not license the JUCE
 * framework to you, and you must discontinue the installation or download
 * process and cease use of the JUCE framework.
 *
 * JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
 * JUCE Privacy Policy: https://juce.com/juce-privacy-policy
 * JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/
 *
 * Or:
 *
 * You may also use this code under the terms of the AGPLv3:
 * https://www.gnu.org/licenses/agpl-3.0.en.html
 *
 * THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
 * WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.
 * ---
 * SPDX-FileCopyrightText: (c) Raw Material Software Limited
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include "plugins/plugin_protocol.h"
#include "utils/file_path_list.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QtQmlIntegration>

namespace zrythm::plugins
{

//==============================================================================

using ProtocolPluginPathsProvider =
  std::function<std::unique_ptr<utils::FilePathList> (Protocol::ProtocolType)>;

class PluginScanManager;

// can't use QObject in nested classes so we put it in a namespace
namespace scanner_private
{
/**
 * @brief A worker thread for the PluginScanManager class.
 *
 * This class is responsible for performing the actual plugin scanning
 * in a separate thread, to avoid blocking the main thread.
 */
class Worker final : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief Constructs a new Worker instance.
   * @param scanner The PluginScanManager instance to work with.
   */
  Worker (PluginScanManager &scanner_);

public:
  /**
   * @brief Starts the plugin scanning process.
   *
   * This slot is called from the PluginScanManager class to initiate the
   * scanning process in the worker thread.
   */
  Q_SLOT void process ();

  /**
   * @brief Request the worker to stop gracefully.
   */
  void requestStop ();

  /**
   * @brief Emitted when the scanning process has finished.
   *
   * This signal is connected to the PluginScanManager class to notify it
   * that the scanning is complete.
   */
  Q_SIGNAL void finished ();

private:
  PluginScanManager &scanner_;
  std::atomic<bool>  should_stop_{ false };
};
} // namespace scanner_private

//==============================================================================

/**
 * @brief Scans for plugins and manages the scanning process.
 *
 * The PluginScanManager class is responsible for scanning the system for
 * available plugins and managing the scanning process. It uses a worker thread
 * to perform the actual scanning, in order to avoid blocking the main thread.
 *
 * The PluginScanManager class provides a QML-friendly API for initiating the
 * scanning process and retrieving information about the currently scanning
 * plugin. It also emits signals to notify the rest of the application when the
 * scanning process has finished and when new plugins have been discovered.
 *
 * The PluginScanManager class is designed to be used in a Qt/QML-based
 * application, and it uses the Qt threading and signal/slot mechanisms to
 * manage the scanning process.
 */
class PluginScanManager final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    QString currentlyScanningPlugin READ getCurrentlyScanningPlugin NOTIFY
      currentlyScanningPluginChanged FINAL)
  QML_UNCREATABLE ("")

  Z_DISABLE_COPY_MOVE (PluginScanManager)

public:
  /**
   * @brief Constructs a PluginScanManager object.
   *
   * @param known_plugins A shared pointer to a juce::KnownPluginList containing
   * previously discovered plugins. A scanner must already be set to it (eg, via
   * setCustomScanner()).
   * @param parent The parent QObject for memory management (optional, defaults
   * to nullptr).
   *
   * This constructor initializes a PluginScanManager with a list of known
   * plugins.
   */
  explicit PluginScanManager (
    std::shared_ptr<juce::KnownPluginList>          known_plugins,
    std::shared_ptr<juce::AudioPluginFormatManager> format_manager,
    ProtocolPluginPathsProvider                     plugin_paths_provider,
    QObject *                                       parent = nullptr);

  ~PluginScanManager () override;

  /**
   * @brief Request the worker thread to stop gracefully.
   */
  Q_INVOKABLE void requestStop ();

  friend class scanner_private::Worker;

  /**
   * @brief To be called by QML to begin scanning.
   */
  Q_INVOKABLE void beginScan ();

  /**
   * @brief Get the currently scanning plugin (property accessor).
   */
  QString getCurrentlyScanningPlugin () const;

  /**
   * @brief Emitted after the scan has finished and after this instance has been
   * moved to the main thread.
   */
  Q_SIGNAL void scanningFinished ();

  /**
   * @brief Emitted from the scan thread (not the main thread) when a new (not
   * known) plugin is added to the list.
   *
   * @note Currently unused.
   */
  // Q_SIGNAL void pluginsAdded ();

  /**
   * @brief Connected to by @ref PluginManager to relay the signal.
   *
   * @warning Do not use directly in other places.
   */
  Q_SIGNAL void currentlyScanningPluginChanged (const QString &plugin);

  /**
   * @brief To be called after scanning is finished to obtain the scan result.
   */
  // auto get_scanned_plugins () const { return known_plugin_list_; }

private:
  void scan_for_plugins ();

  void set_currently_scanning_plugin (const QString &plugin);

  /**
   * @brief Slot to be called by the Worker to let us know plugin scan finished.
   *
   * This is called right before the Worker deletes itself (via deleteLater()).
   */
  Q_SLOT void scan_finished ();

private:
  std::shared_ptr<juce::KnownPluginList>          known_plugin_list_;
  ProtocolPluginPathsProvider                     plugin_paths_provider_;
  std::shared_ptr<juce::AudioPluginFormatManager> format_manager_;

  mutable QMutex currently_scanning_plugin_mutex_;
  QString        currently_scanning_plugin_;

  // Temporaries
  utils::QObjectUniquePtr<QThread>                 scan_thread_;
  utils::QObjectUniquePtr<scanner_private::Worker> worker_;
};

} // namespace zrythm::plugins
