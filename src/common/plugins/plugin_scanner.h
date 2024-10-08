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

#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QtQmlIntegration>

namespace zrythm::plugins
{

class Superprocess final : private juce::ChildProcessCoordinator
{
public:
  Superprocess ();
  ~Superprocess () override;
  ;

  enum class State
  {
    timeout,
    gotResult,
    connectionLost,
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

  JUCE_DECLARE_NON_MOVEABLE (Superprocess)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Superprocess)
};

//==============================================================================

class CustomPluginScanner final
    : public QObject,
      public juce::KnownPluginList::CustomScanner
{
  Q_OBJECT

public:
  CustomPluginScanner (QObject * parent = nullptr);
  ~CustomPluginScanner () override;

  bool findPluginTypesFor (
    juce::AudioPluginFormat                   &format,
    juce::OwnedArray<juce::PluginDescription> &result,
    const juce::String                        &fileOrIdentifier) override;

  /**
   * @brief Used to let Qt know when the scan is done.
   */
  Q_SIGNAL void scanCompleted ();

  // void scanFinished () override;

private:
  /*  Scans for a plugin with format 'formatName' and ID 'fileOrIdentifier'
     using a subprocess, and adds discovered plugin descriptions to 'result'.

      Returns true on success.

      Failure indicates that the subprocess is unrecoverable and should be
     terminated.
  */
  bool addPluginDescriptions (
    const juce::String                        &formatName,
    const juce::String                        &fileOrIdentifier,
    juce::OwnedArray<juce::PluginDescription> &result);

  std::unique_ptr<Superprocess> superprocess;

  JUCE_DECLARE_NON_MOVEABLE (CustomPluginScanner)
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomPluginScanner)
};

//==============================================================================

class PluginScanner;
class PluginScanWorker final : public QObject
{
  Q_OBJECT
public:
  PluginScanWorker (PluginScanner &scanner);

public:
  Q_SLOT void   process ();
  Q_SIGNAL void finished ();

private:
  PluginScanner &scanner_;
};

//==============================================================================

class PluginScanner final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    QString currentlyScanningPlugin READ getCurrentlyScanningPlugin NOTIFY
      currentlyScanningPluginChanged FINAL)

public:
  explicit PluginScanner (QObject * parent = nullptr);
  ~PluginScanner () override;

  friend class PluginScanWorker;

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
  juce::KnownPluginList known_plugin_list_;

  mutable QMutex currently_scanning_plugin_mutex_;
  QString        currently_scanning_plugin_;

  JUCE_DECLARE_NON_COPYABLE (PluginScanner)
  JUCE_DECLARE_NON_MOVEABLE (PluginScanner)
};

} // namespace zrythm::plugins