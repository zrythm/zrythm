// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/gl_context_utils.h"

#include <QOpenGLContext>

namespace zrythm::plugins
{

void
release_current_gl_context ()
{
  if (auto * ctx = QOpenGLContext::currentContext (); ctx != nullptr)
    ctx->doneCurrent ();
}

} // namespace zrythm::plugins
