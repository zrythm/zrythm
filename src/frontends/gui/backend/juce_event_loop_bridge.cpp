// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/gui/backend/juce_event_loop_bridge.h"

#ifdef __linux__

#  include <algorithm>
#  include <map>

#  include "utils/qt.h"

#  include <QSocketNotifier>
#  include <QThread>

#  include <juce_events/native/juce_EventLoopInternal_linux.h>

#endif

namespace zrythm::frontends::gui::backend
{

#ifdef __linux__

class JuceEventLoopBridge::Impl final
    : public QObject,
      public juce::LinuxEventLoopInternal::Listener
{
public:
  Impl ()
  {
    juce::LinuxEventLoopInternal::registerLinuxEventLoopListener (*this);
    sync_notifiers ();
  }

  // The notifiers belong to the thread of this object and are deleted with it
  ~Impl () override
  {
    Q_ASSERT (QThread::currentThread () == thread ());
    juce::LinuxEventLoopInternal::deregisterLinuxEventLoopListener (*this);
  }

  // JUCE calls this on the thread that registers or unregisters a
  // descriptor; the notifiers live on the thread of this object
  void fdCallbacksChanged () override
  {
    QMetaObject::invokeMethod (this, [this] () { sync_notifiers (); });
  }

private:
  void sync_notifiers ()
  {
    const auto fds = juce::LinuxEventLoopInternal::getRegisteredFds ();

    // Erasing deletes the notifier, also while it emits activated() (a JUCE
    // callback may unregister its own descriptor)
    std::erase_if (notifiers_, [&] (const auto &notifier) {
      return !std::ranges::contains (fds, notifier.first);
    });

    for (const int fd : fds)
      {
        if (notifiers_.contains (fd))
          continue;

        auto notifier = utils::make_qobject_unique<QSocketNotifier> (
          static_cast<qintptr> (fd), QSocketNotifier::Read);
        QObject::connect (
          notifier.get (), &QSocketNotifier::activated, this, [fd] () {
            juce::LinuxEventLoopInternal::invokeEventLoopCallbackForFd (fd);
          });
        notifiers_.emplace (fd, std::move (notifier));
      }
  }

  std::map<int, utils::QObjectUniquePtr<QSocketNotifier>> notifiers_;
};

#else

// No-op implementation for platforms where JUCE dispatches events through
// its own message-loop integration
class JuceEventLoopBridge::Impl
{
};

#endif

JuceEventLoopBridge::JuceEventLoopBridge () : pimpl_ (std::make_unique<Impl> ())
{
}

JuceEventLoopBridge::~JuceEventLoopBridge () = default;

} // namespace zrythm::frontends::gui::backend
