// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <juce_dsp/juce_dsp.h>

/**
 * RAII class for managing a DSP context (disabling denormals, etc.).
 */
class DspContextRAII
{
public:
  DspContextRAII () noexcept [[clang::nonblocking]];
  ~DspContextRAII () noexcept [[clang::nonblocking]];

private:
  juce::ScopedNoDenormals ctx_;
};
