// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_DSP_CONTEXT_H__
#define __UTILS_DSP_CONTEXT_H__

#include "zrythm-config.h"

#include "juce_wrapper.h"

/**
 * RAII class for managing a DSP context (disabling denormals, etc.).
 *
 * The DSP context is only started and finished if the
 * `ZRYTHM_USE_OPTIMIZED_DSP` macro is defined.
 */
class DspContextRAII
{
public:
  DspContextRAII ();
  ~DspContextRAII ();

private:
  std::unique_ptr<juce::ScopedNoDenormals> ctx_;
};

#endif // __UTILS_DSP_CONTEXT_H__