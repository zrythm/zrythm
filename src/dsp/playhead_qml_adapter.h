// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/playhead.h"

#include <QtQmlIntegration>

namespace zrythm::dsp
{
class PlayheadQmlWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (double ticks READ ticks WRITE setTicks NOTIFY ticksChanged)
  QML_ELEMENT

public:
  explicit PlayheadQmlWrapper (Playhead &playhead, QObject * parent = nullptr);

  double ticks () const;
  void   setTicks (double ticks);

  Q_SIGNAL void ticksChanged ();

private:
  Q_SLOT void updateTicks ();

private:
  Playhead                       &playhead_;
  double                          last_ticks_ = 0.0;
  utils::QObjectUniquePtr<QTimer> timer_;
};

} // namespace zrythm::dsp
