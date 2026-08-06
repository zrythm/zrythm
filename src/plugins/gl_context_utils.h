// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QOpenGLContext>

namespace zrythm::plugins
{

/**
 * @brief Releases Qt's current GL context on this thread, if any.
 *
 * Must bracket any plug-in UI code that may issue raw GL calls (e.g.,
 * GLX/EGL context binds from DGL/DPF-based editors):
 *
 * - Before: libglvnd rejects a raw glXMakeCurrent with BadAccess when the
 *   calling thread still has a context current from another GL API (e.g.
 *   an EGL context Qt's RHI left current), and the default X error
 *   handler exits the process.
 * - After: QOpenGLContext tracks currency itself and
 *   QRhiGles2::ensureContext() skips re-binding when its context is
 *   "already current" per that tracking; foreign (un)binds are invisible
 *   to it, so subsequent Qt GL operations would run with no context
 *   actually current and silently no-op. Releasing the context forces
 *   the next Qt GL operation to bind for real.
 */
inline void
release_current_gl_context ()
{
  if (auto * ctx = QOpenGLContext::currentContext (); ctx != nullptr)
    ctx->doneCurrent ();
}

/**
 * @brief RAII bracket for foreign (plug-in) UI code that may issue raw GL
 * calls: releases Qt's current GL context on entry and again on exit.
 *
 * See release_current_gl_context() for why both directions are needed.
 */
struct ScopedGlContextRelease
{
  ScopedGlContextRelease () { release_current_gl_context (); }
  ~ScopedGlContextRelease () { release_current_gl_context (); }
};

} // namespace zrythm::plugins
