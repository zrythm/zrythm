// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <string_view>

#if ZRYTHM_TRACY
#  include <tracy/Tracy.hpp>
#else
#  define ZoneScoped
#  define ZoneScopedN(name)
#  define ZoneScopedC(color)
#  define ZoneNamed(varname, active)
#  define ZoneNamedN(varname, name, active)
#  define ZoneText(txt, size)
#  define ZoneTextV(varname, txt, size)
#  define ZoneName(txt, size)
#  define ZoneValue(value)
#  define ZoneColor(color)
#  define FrameMark
#  define FrameMarkNamed(name)
#  define FrameMarkStart(name)
#  define FrameMarkEnd(name)
#  define TracyPlot(name, val)
#  define TracyPlotConfig(name, type, step, fill, color)
#  define TracyMessage(txt, size)
#  define TracyMessageL(txt)
#  define TracyAlloc(ptr, size)
#  define TracyFree(ptr)
#  define TracyAllocN(ptr, size, name)
#  define TracyFreeN(ptr, name)
#endif

class ScopedFrame
{
public:
  explicit ScopedFrame (const char * name) : name_ (name)
  {
    FrameMarkStart (name_);
  }
  ~ScopedFrame () { FrameMarkEnd (name_); }

  ScopedFrame (const ScopedFrame &) = delete;
  ScopedFrame &operator= (const ScopedFrame &) = delete;
  ScopedFrame (ScopedFrame &&) = delete;
  ScopedFrame &operator= (ScopedFrame &&) = delete;

private:
  [[maybe_unused]] const char * name_;
};

namespace zrythm::utils
{

inline bool
is_tracy_initialized ()
{
#if ZRYTHM_TRACY
  return tracy::IsProfilerStarted ();
#else
  return false;
#endif
}

inline void
start_tracy ()
{
#if ZRYTHM_TRACY
  tracy::StartupProfiler ();
#endif
}

inline void
stop_tracy ()
{
#if ZRYTHM_TRACY
  tracy::ShutdownProfiler ();
#endif
}

inline void
set_thread_name (std::string_view name, int32_t group_hint = 0)
{
#if ZRYTHM_TRACY
  tracy::SetThreadNameWithHint (name.data (), group_hint);
#endif
}

} // namespace zrythm::utils
