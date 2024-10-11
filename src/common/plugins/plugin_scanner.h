// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
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

#ifndef ZRYTHM_COMMON_PLUGINS_PLUGIN_SCANNER_H
#define ZRYTHM_COMMON_PLUGINS_PLUGIN_SCANNER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QtQmlIntegration>

namespace zrythm::plugins
{

//==============================================================================

/**
 * @brief Scans for plugins in an out-of-process manner.
 *
 * This class is responsible for scanning for audio plugins in an out-of-process
 * manner, using a separate child process to perform the scanning. This is done
 * to avoid potential crashes or hangs in the main process when scanning for
 * plugins.
 *
 * The scanning is performed asynchronously, and a `scanCompleted()` signal is
 * emitted when the scan is finished.
 */
class OutOfProcessPluginScanner final
    : public QObject,
      public juce::KnownPluginList::CustomScanner
{
  Q_OBJECT

public:
  OutOfProcessPluginScanner (QObject * parent = nullptr);
  ~OutOfProcessPluginScanner () override;

  bool findPluginTypesFor (
    juce::AudioPluginFormat                   &format,
    juce::OwnedArray<juce::PluginDescription> &result,
    const juce::String                        &fileOrIdentifier) override;

  /**
   * @brief Used to let Qt know when the scan is done.
   */
  Q_SIGNAL void scanCompleted ();

private:
  /**
   * @brief Coordinates the execution of a subprocess to scan for plugins.
   *
   * This class is responsible for managing the communication with a child
   * process that is used to scan for audio plugins. It handles sending messages
   * to the child process, receiving responses, and managing the state of the
   * communication.
   *
   * The class is designed to be used in a thread-safe manner, with a mutex and
   * condition variable to synchronize access to the shared state.
   */
  class SubprocessCoordinator final : private juce::ChildProcessCoordinator
  {
  public:
    SubprocessCoordinator ();
    ~SubprocessCoordinator () override;

    enum class State
    {
      Timeout,
      GotResult,
      ConnectionLost,
    };

    struct Response
    {
      State                             state;
      std::unique_ptr<juce::XmlElement> xml;
    };

    Response getResponse ();

    using ChildProcessCoordinator::sendMessageToWorker;

  private:
    void handleMessageFromWorker (const juce::MemoryBlock &mb) override;

    void handleConnectionLost () override;

    std::mutex              mutex;
    std::condition_variable condvar;

    std::unique_ptr<juce::XmlElement> pluginDescription;
    bool                              connectionLost = false;
    bool                              gotResult = false;

    JUCE_DECLARE_NON_MOVEABLE (SubprocessCoordinator)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubprocessCoordinator)
  }; // class Superprocess

  /*  Scans for a plugin with format 'formatName' and ID 'fileOrIdentifier'
     using a subprocess, and adds discovered plugin descriptions to 'result'.

      Returns true on success.

      Failure indicates that the subprocess is unrecoverable and should be
     terminated.
  */
  bool add_plugin_descriptions (
    juce::AudioPluginFormat                   &format,
    const juce::String                        &file_or_identifier,
    juce::OwnedArray<juce::PluginDescription> &result);

  std::unique_ptr<SubprocessCoordinator> coordinator_;

  JUCE_DECLARE_NON_MOVEABLE (OutOfProcessPluginScanner)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutOfProcessPluginScanner)
};

//==============================================================================

class PluginScanner;

// can't use QObject in nested classes so we put it in a namespace
namespace scanner_private
{
/**
 * @brief A worker thread for the PluginScanner class.
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
   * @param scanner The PluginScanner instance to work with.
   */
  Worker (PluginScanner &scanner_);

public:
  /**
   * @brief Starts the plugin scanning process.
   *
   * This slot is called from the PluginScanner class to initiate the
   * scanning process in the worker thread.
   */
  Q_SLOT void process ();

  /**
   * @brief Emitted when the scanning process has finished.
   *
   * This signal is connected to the PluginScanner class to notify it
   * that the scanning is complete.
   */
  Q_SIGNAL void finished ();

private:
  PluginScanner &scanner_;
};
} // namespace scanner_private

//==============================================================================

/**
 * @brief Scans for plugins and manages the scanning process.
 *
 * The PluginScanner class is responsible for scanning the system for available
 * plugins and managing the scanning process. It uses a worker thread to perform
 * the actual scanning, in order to avoid blocking the main thread.
 *
 * The PluginScanner class provides a QML-friendly API for initiating the
 * scanning process and retrieving information about the currently scanning
 * plugin. It also emits signals to notify the rest of the application when the
 * scanning process has finished and when new plugins have been discovered.
 *
 * The PluginScanner class is designed to be used in a Qt/QML-based application,
 * and it uses the Qt threading and signal/slot mechanisms to manage the
 * scanning process.
 */
class PluginScanner final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    QString currentlyScanningPlugin READ getCurrentlyScanningPlugin NOTIFY
      currentlyScanningPluginChanged FINAL)

public:
  /**
   * @brief Constructs a PluginScanner object.
   *
   * @param known_plugins A shared pointer to a juce::KnownPluginList containing
   * previously discovered plugins.
   * @param parent The parent QObject for memory management (optional, defaults
   * to nullptr).
   *
   * This constructor initializes a PluginScanner with a list of known plugins.
   */
  explicit PluginScanner (
    std::shared_ptr<juce::KnownPluginList> known_plugins,
    QObject *                              parent = nullptr);

  ~PluginScanner () override;

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
  Q_SIGNAL void pluginsAdded ();

  /**
   * @brief Connected to by @ref PluginManager to relay the signal.
   *
   * @warning Do not use directly in other places.
   */
  Q_SIGNAL void currentlyScanningPluginChanged (const QString &plugin);

  /**
   * @brief Automatically called when the scan thread finishes.
   */
  Q_SLOT void scan_finished ();

private:
  void scan_for_plugins ();

  void set_currently_scanning_plugin (const QString &plugin);

private:
  std::shared_ptr<juce::KnownPluginList> known_plugin_list_;

  mutable QMutex currently_scanning_plugin_mutex_;
  QString        currently_scanning_plugin_;

  JUCE_DECLARE_NON_COPYABLE (PluginScanner)
  JUCE_DECLARE_NON_MOVEABLE (PluginScanner)
};

} // namespace zrythm::plugins

#endif // ZRYTHM_COMMON_PLUGINS_PLUGIN_SCANNER_H