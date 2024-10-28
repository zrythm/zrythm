// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once
#include <QList>
#include <QMutex>
#include <QObject>
#include <QTimer>

class RealtimeUpdater : public QObject
{
  Q_OBJECT
public:
  static RealtimeUpdater &instance ()
  {
    static RealtimeUpdater instance;
    return instance;
  }

  ~RealtimeUpdater () override;

  void registerObject (QObject * obj);

  void deregisterObject (QObject * obj);

private:
  RealtimeUpdater (QObject * parent = nullptr);

  void processUpdates ();

  QTimer           timer_;
  QList<QObject *> objects_;
  QMutex           mutex_;
};
