// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{
/**
 * @brief QML-exposed wrapper over a QFuture: progress properties for
 * binding, plus an outcome when the task completes.
 *
 * Exactly one of succeeded(), failed() or canceled() is emitted per
 * future. Outcomes are emitted on a later event loop pass than the
 * wrapper's construction, so consumers attaching in the same iteration
 * the wrapper is received never miss one. Cancellation is not a
 * failure. resultVar() is only valid after succeeded().
 */
class QFutureQmlWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    int progressMinimum READ progressMinimum NOTIFY progressRangeChanged)
  Q_PROPERTY (
    int progressMaximum READ progressMaximum NOTIFY progressRangeChanged)
  Q_PROPERTY (int progressValue READ progressValue NOTIFY progressValueChanged)
  Q_PROPERTY (QString progressText READ progressText NOTIFY progressTextChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ~QFutureQmlWrapper () override = default;

  virtual int                  progressMinimum () const = 0;
  virtual int                  progressMaximum () const = 0;
  virtual int                  progressValue () const = 0;
  virtual QString              progressText () const = 0;
  Q_INVOKABLE virtual QVariant resultVar () const = 0;
  Q_INVOKABLE virtual void     cancel () = 0;

Q_SIGNALS:
  void progressValueChanged (int value);
  void progressRangeChanged (int minimum, int maximum);
  void progressTextChanged (const QString &text);
  void succeeded ();
  void failed (const QString &errorString);
  void canceled ();

protected:
  void setup (QFutureWatcherBase &watcher) const
  {
    QObject::connect (
      &watcher, &QFutureWatcherBase::progressRangeChanged, this,
      &QFutureQmlWrapper::progressRangeChanged);
    QObject::connect (
      &watcher, &QFutureWatcherBase::progressValueChanged, this,
      &QFutureQmlWrapper::progressValueChanged);
    QObject::connect (
      &watcher, &QFutureWatcherBase::progressTextChanged, this,
      &QFutureQmlWrapper::progressTextChanged);
  }
};

template <typename T> class QFutureQmlWrapperT : public QFutureQmlWrapper
{
public:
  QFutureQmlWrapperT (QFuture<T> future) : future_ (future)
  {
    QObject::connect (&watcher_, &QFutureWatcherBase::finished, this, [this] () {
      resolve_outcome ();
    });
    setup (watcher_);
    watcher_.setFuture (future_);
  }

  int progressMinimum () const override { return watcher_.progressMinimum (); }
  int progressMaximum () const override { return watcher_.progressMaximum (); }
  int progressValue () const override { return watcher_.progressValue (); }
  QString  progressText () const override { return watcher_.progressText (); }
  QVariant resultVar () const override
  {
    if constexpr (std::is_same_v<T, void>)
      {
        return QVariant{};
      }
    else
      {
        assert (future_.isResultReadyAt (0));
        return QVariant::fromValue (watcher_.result ());
      }
  }

  void cancel () override { future_.cancel (); }

private:
  // Runs when the future reaches its terminal state. A stored exception
  // also puts the future into the canceled state, so isCanceled() alone
  // cannot distinguish an exception from a user cancel;
  // waitForFinished() is the probe: on a finished future it returns
  // immediately and rethrows only when an exception is stored (result()
  // is undefined behavior on a canceled future without results, so it
  // cannot probe)
  void resolve_outcome ()
  {
    if (resolved_)
      return;

    resolved_ = true;

    try
      {
        future_.waitForFinished ();
      }
    catch (const std::exception &e)
      {
        const auto error = QString::fromUtf8 (e.what ());
        emit_outcome_later ([this, error] () { Q_EMIT failed (error); });
        return;
      }
    catch (...)
      {
        emit_outcome_later ([this] () {
          Q_EMIT failed (QCoreApplication::tr ("Unknown error"));
        });
        return;
      }

    if (future_.isCanceled ())
      {
        emit_outcome_later ([this] () { Q_EMIT canceled (); });
        return;
      }

    emit_outcome_later ([this] () { Q_EMIT succeeded (); });
  }

  // Queues the outcome so it is never delivered during the event loop
  // iteration the wrapper is created in
  template <typename Emit> void emit_outcome_later (Emit emitter)
  {
    QMetaObject::invokeMethod (this, emitter, Qt::QueuedConnection);
  }

  QFuture<T>        future_;
  QFutureWatcher<T> watcher_;
  bool              resolved_ = false;
};
}
