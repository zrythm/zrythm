// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/juce_event_loop_bridge.h"

#ifdef __linux__

#  include <algorithm>
#  include <map>

#  include "utils/qt.h"

#  include <QSocketNotifier>

#  include <juce_events/native/juce_EventLoopInternal_linux.h>

namespace zrythm::gui::backend
{

class JuceEventLoopBridge::Impl final
    : public juce::LinuxEventLoopInternal::Listener
{
public:
  Impl ()
  {
    juce::LinuxEventLoopInternal::registerLinuxEventLoopListener (*this);
    sync_notifiers ();
  }

  ~Impl () override
  {
    juce::LinuxEventLoopInternal::deregisterLinuxEventLoopListener (*this);
  }

  void fdCallbacksChanged () override { sync_notifiers (); }

private:
  void sync_notifiers ()
  {
    const auto fds = juce::LinuxEventLoopInternal::getRegisteredFds ();

    for (auto it = notifiers_.begin (); it != notifiers_.end ();)
      {
        if (std::ranges::contains (fds, it->first))
          {
            ++it;
            continue;
          }
        // Deferred deletion: this runs during JUCE callbacks that may
        // unregister descriptors, and a notifier must not be deleted while
        // its activated signal is being emitted
        auto * notifier = it->second.release ();
        notifier->setEnabled (false);
        notifier->deleteLater ();
        it = notifiers_.erase (it);
      }

    for (const int fd : fds)
      {
        if (notifiers_.contains (fd))
          continue;

        auto notifier = utils::make_qobject_unique<QSocketNotifier> (
          static_cast<qintptr> (fd), QSocketNotifier::Read);
        QObject::connect (
          notifier.get (), &QSocketNotifier::activated, notifier.get (), [fd] () {
            juce::LinuxEventLoopInternal::invokeEventLoopCallbackForFd (fd);
          });
        notifiers_.emplace (fd, std::move (notifier));
      }
  }

  std::map<int, utils::QObjectUniquePtr<QSocketNotifier>> notifiers_;
};

JuceEventLoopBridge::JuceEventLoopBridge () : pimpl_ (std::make_unique<Impl> ())
{
}

JuceEventLoopBridge::~JuceEventLoopBridge () = default;

#else

class JuceEventLoopBridge::Impl
{
};

JuceEventLoopBridge::JuceEventLoopBridge () : pimpl_ (std::make_unique<Impl> ())
{
}

JuceEventLoopBridge::~JuceEventLoopBridge () = default;

#endif

} // namespace zrythm::gui::backend
