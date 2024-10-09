// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <QCoreApplication>

#include "engine/audio_engine.h"
#include <fftw3.h>

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  QCoreApplication app (argc, argv);
  qDebug () << "Starting audio engine...";

// TODO
#if 0
#  if HAVE_X11
  /* init xlib threads */
  z_info ("Initing X threads...");
  XInitThreads ();
#  endif

  /* init fftw */
  z_info ("Making fftw planner thread safe...");
  fftw_make_planner_thread_safe ();
  fftwf_make_planner_thread_safe ();
#endif

  engine::AudioEngine engine;
  return app.exec ();
}
