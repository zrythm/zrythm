// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QtQmlIntegration>

namespace zrythm::gui::qquick
{
/**
 * @brief QML-exposed wrapper over a QFuture that provides properties for
 * binding.
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
  Q_INVOKABLE virtual bool     isCanceled () const = 0;

Q_SIGNALS:
  void progressValueChanged (int value);
  void progressRangeChanged (int minimum, int maximum);
  void progressTextChanged (const QString &text);
  void finished ();

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
    QObject::connect (
      &watcher, &QFutureWatcherBase::finished, this,
      &QFutureQmlWrapper::finished);
  }
};

template <typename T> class QFutureQmlWrapperT : public QFutureQmlWrapper
{
public:
  QFutureQmlWrapperT (QFuture<T> future) : future_ (future)
  {
    watcher_.setFuture (future_);
    setup (watcher_);
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
  bool isCanceled () const override { future_.isCanceled (); }

private:
  QFuture<T>        future_;
  QFutureWatcher<T> watcher_;
};
}
