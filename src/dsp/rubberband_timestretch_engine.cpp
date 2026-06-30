// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>

#include "dsp/rubberband_timestretch_engine.h"
#include "utils/logger.h"

#include <rubberband/RubberBandStretcher.h>

namespace zrythm::dsp
{

namespace
{
using RBS = RubberBand::RubberBandStretcher;

/// Build the RubberBand option flags for an offline stretch.
/// Polyphonic/Repitch use the R3 (Finer) engine; Beats/Monophonic use the
/// R2 (Standard) engine with material-appropriate detector and transient
/// options.
RBS::Options
make_options (const StretchOptions &opts)
{
  RBS::Options o = RBS::OptionProcessOffline;

  switch (opts.algorithm)
    {
    case StretchOptions::Algorithm::Beats:
      // R2 engine with percussive detection and crisp transients.
      o = static_cast<RBS::Options> (
        o | RBS::OptionEngineFaster | RBS::OptionDetectorPercussive
        | RBS::OptionTransientsCrisp);
      break;
    case StretchOptions::Algorithm::Monophonic:
      // R2 engine with soft detection and smooth transients.
      o = static_cast<RBS::Options> (
        o | RBS::OptionEngineFaster | RBS::OptionDetectorSoft
        | RBS::OptionTransientsSmooth);
      break;
    case StretchOptions::Algorithm::Polyphonic:
      // R3 (Finer) engine — highest quality; ignores transient/detector flags.
      o = static_cast<RBS::Options> (o | RBS::OptionEngineFiner);
      break;
    case StretchOptions::Algorithm::Repitch:
      // Unreachable: supports() returns false for Repitch, so the NVI layer
      // rejects it before stretch_impl() is entered. Kept defensively in case
      // stretch_impl() is ever called directly; falls back to Polyphonic (R3).
      z_warning (
        "Repitch algorithm reached stretch_impl() despite being unsupported "
        "by RubberBand; falling back to Polyphonic (R3)");
      o = static_cast<RBS::Options> (o | RBS::OptionEngineFiner);
      break;
    }

  if (
    opts.preserve_formants
    && opts.algorithm != StretchOptions::Algorithm::Repitch)
    o = static_cast<RBS::Options> (o | RBS::OptionFormantPreserved);

  return o;
}
} // namespace

RubberBandTimeStretchEngine::RubberBandTimeStretchEngine (
  units::sample_rate_t sample_rate)
    : sample_rate_ (sample_rate)
{
}

RubberBandTimeStretchEngine::~RubberBandTimeStretchEngine () = default;

std::string_view
RubberBandTimeStretchEngine::id () const
{
  return "rubberband";
}

bool
RubberBandTimeStretchEngine::supports (StretchOptions::Algorithm algorithm) const
{
  // Repitch (varispeed) is not implementable with RubberBand, which always
  // preserves pitch. Reject it so the NVI layer throws rather than silently
  // producing pitch-preserved output. A future Resampler engine will provide
  // true varispeed.
  return algorithm != StretchOptions::Algorithm::Repitch;
}

utils::audio::AudioBuffer
RubberBandTimeStretchEngine::stretch_impl (
  const utils::audio::AudioBuffer &input,
  const TimeWarpMap               &warp,
  const StretchOptions            &options)
{
  // The NVI layer in ITimeStretchEngine::stretch() has already validated
  // warp.is_valid(), the source-length match, and supports(). Repitch is
  // rejected by supports(), so only Beats/Monophonic/Polyphonic reach here.

  const int    channels = input.getNumChannels ();
  const size_t source_frames = static_cast<size_t> (input.getNumSamples ());
  const size_t output_frames =
    static_cast<size_t> (warp.output_length.in (units::samples));
  const double time_ratio =
    static_cast<double> (output_frames) / static_cast<double> (source_frames);

  RBS stretcher (
    static_cast<size_t> (sample_rate_.in (units::sample_rate)),
    static_cast<size_t> (channels), make_options (options), time_ratio, 1.0);

  // Tell the offline stretcher the exact input length so the output length is
  // exact as well.
  stretcher.setExpectedInputDuration (source_frames);

  // Seed the variable stretch profile from the warp anchors (source frame →
  // output frame). Must be called before the first process() and after the
  // time ratio is set.
  std::map<size_t, size_t> keyframes;
  for (const auto &anchor : warp.anchors)
    {
      const auto sf =
        static_cast<size_t> (anchor.source_frame.in (units::samples));
      const auto of =
        static_cast<size_t> (anchor.output_frame.in (units::samples));
      // RubberBand's updateRatioFromMap() computes the initial time ratio as
      // output/source of the first keyframe.  Identity anchors (sf == of)
      // yield ratio 1.0, which silently disables stretching for the entire
      // output.  Skip them so the first keyframe carries the real stretch
      // ratio.
      if (sf == 0 || sf == of)
        continue;
      keyframes.emplace (sf, of);
    }
  stretcher.setKeyFrameMap (keyframes);

  // Reusable channel pointer buffers — avoid per-block allocation.
  std::vector<const float *> read_ptrs (static_cast<size_t> (channels));
  std::vector<float *>       write_ptrs (static_cast<size_t> (channels));

  const auto set_read_ptrs = [&] (size_t offset) {
    for (int c = 0; c < channels; ++c)
      read_ptrs[static_cast<size_t> (c)] =
        input.getReadPointer (c) + static_cast<int> (offset);
  };

  // Offline requires two passes: study() (builds the stretch profile) then
  // process() (applies it).  study() accepts the entire input at once.
  // process() uses getSamplesRequired() so RubberBand manages its own block
  // sizing.
  set_read_ptrs (0);
  stretcher.study (read_ptrs.data (), source_frames, true);

  // Retrieve output interleaved with process blocks so the stretcher's output
  // buffer never grows unbounded.
  utils::audio::AudioBuffer output (channels, static_cast<int> (output_frames));
  size_t                    retrieved_total = 0;
  const auto                drain = [&] () {
    while (retrieved_total < output_frames)
      {
        const int avail = stretcher.available ();
        if (avail <= 0)
          return; // -1: finished; 0: need more input
        const size_t want = std::min (
          static_cast<size_t> (avail), output_frames - retrieved_total);
        for (int c = 0; c < channels; ++c)
          write_ptrs[static_cast<size_t> (c)] =
            output.getWritePointer (c) + static_cast<int> (retrieved_total);
        const size_t got = stretcher.retrieve (write_ptrs.data (), want);
        if (got == 0)
          return;
        retrieved_total += got;
      }
  };

  for (size_t offset = 0; offset < source_frames;)
    {
      const size_t required =
        std::min (stretcher.getSamplesRequired (), source_frames - offset);
      const bool final = offset + required == source_frames;
      set_read_ptrs (offset);
      stretcher.process (read_ptrs.data (), required, final);
      drain ();
      offset += required;
    }
  drain ();

  // Defensive: if offline produced fewer frames than expected, silence the
  // remainder so the contract (exactly output_length frames) holds.
  if (retrieved_total < output_frames)
    output.clear (
      0, static_cast<int> (retrieved_total),
      static_cast<int> (output_frames - retrieved_total));

  return output;
}

} // namespace zrythm::dsp
