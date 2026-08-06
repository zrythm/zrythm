// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <unordered_map>

#include "plugins/gl_context_utils.h"
#include "plugins/plugin_run_loop.h"
#include "utils/qt.h"

#include <QSocketNotifier>
#include <QTimer>

namespace zrythm::plugins
{

class PluginRunLoop::Impl
{
public:
  explicit Impl (PluginRunLoop &owner) : owner_ (owner) { }

  struct FdWatch
  {
    int                                      fd = -1;
    PluginRunLoop::FdCallback                callback;
    utils::QObjectUniquePtr<QSocketNotifier> read_notifier;
    utils::QObjectUniquePtr<QSocketNotifier> write_notifier;
  };

  struct TimerEntry
  {
    PluginRunLoop::TimerCallback    callback;
    utils::QObjectUniquePtr<QTimer> timer;
  };

  /**
   * @brief Enables/disables the watch's notifiers per the given flags,
   * creating them on first use.
   */
  void apply_fd_flags (FdWatch &watch, bool read, bool write)
  {
    if (read && watch.read_notifier == nullptr)
      {
        watch.read_notifier = utils::make_qobject_unique<QSocketNotifier> (
          static_cast<qintptr> (watch.fd), QSocketNotifier::Read, &owner_);
        QObject::connect (
          watch.read_notifier.get (), &QSocketNotifier::activated, &owner_,
          [cb = watch.callback] {
            const ScopedGlContextRelease gl_release;
            cb (true);
          });
      }
    if (write && watch.write_notifier == nullptr)
      {
        watch.write_notifier = utils::make_qobject_unique<QSocketNotifier> (
          static_cast<qintptr> (watch.fd), QSocketNotifier::Write, &owner_);
        QObject::connect (
          watch.write_notifier.get (), &QSocketNotifier::activated, &owner_,
          [cb = watch.callback] {
            const ScopedGlContextRelease gl_release;
            cb (false);
          });
      }
    if (watch.read_notifier != nullptr)
      watch.read_notifier->setEnabled (read);
    if (watch.write_notifier != nullptr)
      watch.write_notifier->setEnabled (write);
  }

  PluginRunLoop       &owner_;
  PluginRunLoop::Token next_token_ = 0;
  std::unordered_map<PluginRunLoop::Token, std::unique_ptr<FdWatch>> fd_watches_;
  std::unordered_map<PluginRunLoop::Token, std::unique_ptr<TimerEntry>> timers_;
};

PluginRunLoop::PluginRunLoop (QObject * parent)
    : QObject (parent), pimpl_ (std::make_unique<Impl> (*this))
{
}

PluginRunLoop::~PluginRunLoop () = default;

PluginRunLoop::Token
PluginRunLoop::register_fd (int fd, bool read, bool write, FdCallback callback)
{
  auto watch = std::make_unique<Impl::FdWatch> ();
  watch->fd = fd;
  watch->callback = std::move (callback);
  pimpl_->apply_fd_flags (*watch, read, write);

  const auto token = pimpl_->next_token_++;
  pimpl_->fd_watches_.emplace (token, std::move (watch));
  return token;
}

void
PluginRunLoop::update_fd (Token token, bool read, bool write)
{
  const auto it = pimpl_->fd_watches_.find (token);
  Q_ASSERT (it != pimpl_->fd_watches_.end ());
  if (it == pimpl_->fd_watches_.end ())
    return;

  pimpl_->apply_fd_flags (*it->second, read, write);
}

void
PluginRunLoop::unregister_fd (Token token)
{
  pimpl_->fd_watches_.erase (token);
}

PluginRunLoop::Token
PluginRunLoop::register_timer (
  std::chrono::milliseconds interval,
  TimerCallback             callback)
{
  auto entry = std::make_unique<Impl::TimerEntry> ();
  entry->callback = std::move (callback);
  entry->timer = utils::make_qobject_unique<QTimer> (this);
  connect (entry->timer.get (), &QTimer::timeout, this, [cb = entry->callback] {
    const ScopedGlContextRelease gl_release;
    cb ();
  });
  entry->timer->start (interval);

  const auto token = pimpl_->next_token_++;
  pimpl_->timers_.emplace (token, std::move (entry));
  return token;
}

void
PluginRunLoop::unregister_timer (Token token)
{
  pimpl_->timers_.erase (token);
}

void
PluginRunLoop::clear ()
{
  pimpl_->fd_watches_.clear ();
  pimpl_->timers_.clear ();
}

} // namespace zrythm::plugins
