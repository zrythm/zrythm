// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <unordered_map>

#include "plugins/plugin_run_loop.h"

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
    int                              fd = -1;
    PluginRunLoop::FdCallback        callback;
    std::unique_ptr<QSocketNotifier> read_notifier;
    std::unique_ptr<QSocketNotifier> write_notifier;
  };

  struct TimerEntry
  {
    PluginRunLoop::TimerCallback callback;
    std::unique_ptr<QTimer>      timer;
  };

  /**
   * @brief Enables/disables the watch's notifiers per the given flags,
   * creating them on first use.
   */
  void apply_fd_flags (FdWatch &watch, bool read, bool write)
  {
    if (read && watch.read_notifier == nullptr)
      {
        watch.read_notifier = std::make_unique<QSocketNotifier> (
          static_cast<qintptr> (watch.fd), QSocketNotifier::Read, &owner_);
        QObject::connect (
          watch.read_notifier.get (), &QSocketNotifier::activated, &owner_,
          [cb = watch.callback] { cb (true); });
      }
    if (write && watch.write_notifier == nullptr)
      {
        watch.write_notifier = std::make_unique<QSocketNotifier> (
          static_cast<qintptr> (watch.fd), QSocketNotifier::Write, &owner_);
        QObject::connect (
          watch.write_notifier.get (), &QSocketNotifier::activated, &owner_,
          [cb = watch.callback] { cb (false); });
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
  entry->timer = std::make_unique<QTimer> (this);
  connect (entry->timer.get (), &QTimer::timeout, this, entry->callback);
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

} // namespace zrythm::plugins
