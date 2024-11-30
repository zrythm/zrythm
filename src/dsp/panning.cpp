// SPDX-FileCopyrightText: © 2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <numbers>

#include "dsp/panning.h"
#include "utils/exceptions.h"

namespace zrythm::dsp
{

std::pair<float, float>
calculate_panning (PanLaw law, PanAlgorithm algo, float pan)
{
  // Clamp pan value between 0 and 1
  constexpr float EPSILON = 0.0000001f;
  pan = std::clamp (pan, EPSILON, 1.f - EPSILON);

  float left_gain = 0.0f;
  float right_gain = 0.0f;

  // Calculate base gains based on algorithm
  switch (algo)
    {
    case PanAlgorithm::Linear:
      left_gain = 1.0f - pan;
      right_gain = pan;
      break;

    case PanAlgorithm::SquareRoot:
      left_gain = std::sqrt (1.0f - pan);
      right_gain = std::sqrt (pan);
      break;

    case PanAlgorithm::SineLaw:
      // Convert pan [0,1] to radians [0,π/2]
      const float angle = pan * M_PI_2f;
      left_gain = std::cos (angle);
      right_gain = std::sin (angle);
      break;
    }

  constexpr float PAN_MINUS_3DB_AMP = -0.292054216f; //< -3dBfs (0.707945784f).
  constexpr float PAN_MINUS_6DB_AMP = -0.498812766f; //< -6dBfs (0.501187234f).

  // Apply pan law compensation for mono->stereo conversion
  float compensation = 1.0f;
  switch (law)
    {
    case PanLaw::ZerodB:
      compensation = 1.0f;
      break;

    case PanLaw::Minus3dB:
      compensation += PAN_MINUS_3DB_AMP;
      break;

    case PanLaw::Minus6dB:
      compensation += PAN_MINUS_6DB_AMP;
      break;
    }

  return { left_gain * compensation, right_gain * compensation };
}

}; // namespace zrythm::dsp
