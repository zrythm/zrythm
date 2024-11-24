// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/dsp_context.h"
#include "utils/logger.h"

DspContextRAII::DspContextRAII ()
{
  z_info ("Starting DSP context...");

  ctx_ = std::make_unique<juce::ScopedNoDenormals> ();
}

DspContextRAII::~DspContextRAII ()
{
  z_info ("Destroying DSP context...");
}
