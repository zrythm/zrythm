// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_LSP_DSP_CONTEXT_H__
#define __UTILS_LSP_DSP_CONTEXT_H__

#include "zrythm-config.h"

#include <memory>

#if HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif

/**
 * RAII class to manage the lifecycle of an LSP DSP context.
 *
 * This class is responsible for starting and finishing the DSP context
 * when the object is constructed and destructed, respectively. It
 * provides access to the underlying DSP context through the `get()`
 * method.
 *
 * The DSP context is only started and finished if the
 * `ZRYTHM_USE_OPTIMIZED_DSP` macro is defined.
 */
class LspDspContextRAII
{
#if HAVE_LSP_DSP
public:
  LspDspContextRAII ();
  ~LspDspContextRAII ();

private:
  std::unique_ptr<lsp::dsp::context_t> ctx_;
#endif
};

#endif // __UTILS_LSP_DSP_CONTEXT_H__