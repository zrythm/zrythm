// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "utils/types.h"

#ifdef __linux__
#  define ENABLE_THREAD_SAFE_FFTW 1
#else
#  define ENABLE_THREAD_SAFE_FFTW 0
#endif

#if ENABLE_THREAD_SAFE_FFTW
#  include <dlfcn.h>
#endif

class ThreadSafeFFTW
{
public:
  ThreadSafeFFTW ()
  {
#if ENABLE_THREAD_SAFE_FFTW
    load_and_make_thread_safe (
      "libfftw3_threads.so.3", "fftw_make_planner_thread_safe");
    load_and_make_thread_safe (
      "libfftw3f_threads.so.3", "fftwf_make_planner_thread_safe");
    load_and_make_thread_safe (
      "libfftw3l_threads.so.3", "fftwl_make_planner_thread_safe");
    load_and_make_thread_safe (
      "libfftw3q_threads.so.3", "fftwq_make_planner_thread_safe");
#endif
  }

  Z_DISABLE_COPY_MOVE (ThreadSafeFFTW)

private:
#if ENABLE_THREAD_SAFE_FFTW
  struct LibHandleDeleter
  {
    void operator() (void * handle) const noexcept
    {
      if (handle)
        dlclose (handle);
    }
  };

  using LibHandle = std::unique_ptr<void, LibHandleDeleter>;

  void load_and_make_thread_safe (const char * libname, const char * funcname)
  {
    LibHandle lib (dlopen (libname, RTLD_LAZY));
    if (!lib)
      return;

    using ThreadSafeFunc = void (*) ();
    if (
      auto func = reinterpret_cast<ThreadSafeFunc> (dlsym (lib.get (), funcname)))
      {
        func ();
      }

    // Keep the library loaded
    lib.release (); // Leak intentionally - we need these to stay loaded
  }
#endif
};
