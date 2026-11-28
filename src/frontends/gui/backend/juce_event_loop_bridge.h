// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include <QtGlobal>

namespace zrythm::frontends::gui::backend
{

/**
 * @brief Dispatches JUCE's main-thread events inside Qt's event loop.
 *
 * On GNU/Linux, JUCE delivers messages and registered file-descriptor
 * events (including X11 events for JUCE-created windows) through a set of
 * polled file descriptors. This class mirrors those descriptors into
 * QSocketNotifier watches and forwards readiness to JUCE, so JUCE events
 * are dispatched whenever Qt's event loop wakes up, without polling.
 *
 * Only functional on GNU/Linux; a no-op on other platforms.
 *
 * Must be created after JUCE is initialized and destroyed before JUCE
 * shuts down (before juce::ScopedJuceInitialiser_GUI is destroyed).
 *
 * Must be created and destroyed on the thread that runs the Qt event loop
 * dispatching JUCE events. JUCE registers and unregisters descriptors on
 * any thread; the bridge applies those changes on its own thread.
 */
class JuceEventLoopBridge final
{
public:
  JuceEventLoopBridge ();
  ~JuceEventLoopBridge ();

  Q_DISABLE_COPY_MOVE (JuceEventLoopBridge)

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

} // namespace zrythm::frontends::gui::backend
