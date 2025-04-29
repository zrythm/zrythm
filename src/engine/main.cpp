// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "utils/logger.h"

#include <QCoreApplication>

#include "engine/audio_engine_application.h"

#if 0
/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  zrythm::engine::AudioEngineApplication app (argc, argv);
  z_info ("Starting audio engine...");

// TODO
#  if HAVE_X11 && false
  /* init xlib threads */
  z_info ("Initing X threads...");
  XInitThreads ();
#  endif

  /* init fftw */
  z_info ("Making fftw planner thread safe...");
  fftw_make_planner_thread_safe ();
  fftwf_make_planner_thread_safe ();

  return app.exec ();
}
#endif

START_JUCE_APPLICATION (zrythm::engine::AudioEngineApplication)
