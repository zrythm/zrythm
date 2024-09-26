// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <QCoreApplication>

#include "engine/audio_engine.h"

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  QCoreApplication app (argc, argv);
  qDebug () << "Starting audio engine...";
  engine::AudioEngine engine;
  return app.exec ();
}
