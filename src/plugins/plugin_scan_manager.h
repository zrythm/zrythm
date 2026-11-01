// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
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

#include <cstdint>

#include "plugins/plugin_catalog.h"
#include "plugins/plugin_protocol.h"
#include "plugins/plugin_scan_format.h"
#include "plugins/plugin_scan_protocol.h"
#include "utils/file_path_list.h"
#include "utils/qt.h"

#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QThread>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::plugins
{

using discovery::PluginCatalog;
using discovery::PluginScanner;

//==============================================================================

using ProtocolPluginPathsProvider =
  std::function<std::unique_ptr<utils::FilePathList> (Protocol::ProtocolType)>;

class PluginScanManager;

namespace scanner_private
{
/**
 * @brief Worker that scans plugin identifiers outside the main thread.
 */
class Worker final : public QObject
{
  Q_OBJECT

public:
  Worker (
    PluginScanManager                       &scanner_,
    std::shared_ptr<PluginCatalog>           catalog,
    std::shared_ptr<PluginScanFormatManager> format_manager,
    PluginScanner *                          plugin_scanner,
    std::uint64_t                            scan_generation);

public:
  Q_SLOT void process ();

  void requestStop ();

  Q_SIGNAL void finished ();

private:
  void process_scan ();

  PluginScanManager                       &scanner_;
  std::shared_ptr<PluginCatalog>           catalog_;
  std::shared_ptr<PluginScanFormatManager> format_manager_;
  PluginScanner *                          plugin_scanner_;
  std::uint64_t                            scan_generation_;
  std::atomic<bool>                        should_stop_{ false };
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

  Q_DISABLE_COPY_MOVE (PluginScanManager)

public:
  PluginScanManager (
    std::shared_ptr<PluginCatalog>           catalog,
    std::shared_ptr<PluginScanFormatManager> format_manager,
    ProtocolPluginPathsProvider              plugin_paths_provider,
    std::unique_ptr<PluginScanner>           plugin_scanner,
    QObject *                                parent = nullptr);

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

  Q_SIGNAL void scanningFinished ();

  Q_SIGNAL void currentlyScanningPluginChanged (const QString &plugin);

private:
  void set_currently_scanning_plugin (const QString &plugin);

  Q_SLOT void scan_finished ();

  /** Starts a new scan generation. Must be called with no scan thread active. */
  void start_scan ();

  /** Discards a finished scan generation and starts a requested restart. */
  void finish_scan (std::uint64_t scan_generation);

private:
  std::shared_ptr<PluginCatalog>           catalog_;
  ProtocolPluginPathsProvider              plugin_paths_provider_;
  std::shared_ptr<PluginScanFormatManager> format_manager_;
  std::unique_ptr<PluginScanner>           plugin_scanner_;

  mutable QMutex currently_scanning_plugin_mutex_;
  QString        currently_scanning_plugin_;

  std::uint64_t active_generation_ = 0;
  bool          restart_pending_ = false;

  // Temporaries
  utils::QObjectUniquePtr<QThread>                 scan_thread_;
  utils::QObjectUniquePtr<scanner_private::Worker> worker_;
};

} // namespace zrythm::plugins
