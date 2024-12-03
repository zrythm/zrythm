#include "dsp/ditherer.h"
#include "utils/gtest_wrapper.h"

TEST (DithererTest, Reset)
{
  zrythm::dsp::Ditherer ditherer;

  // Test with 16-bit
  ditherer.reset (16);

  // Test with 24-bit
  ditherer.reset (24);

  // Test with 8-bit
  ditherer.reset (8);
}

TEST (DithererTest, ProcessZeroSamples)
{
  zrythm::dsp::Ditherer ditherer;
  ditherer.reset (16);

  float     samples[] = { 0.0f, 0.0f, 0.0f, 0.0f };
  const int num_samples = 4;

  ditherer.process (samples, num_samples);

  // Verify samples remain very close to zero
  for (float sample : samples)
    {
      EXPECT_NEAR (sample, 0.0f, 0.000001f);
    }
}

TEST (DithererTest, ProcessNonZeroSamples)
{
  zrythm::dsp::Ditherer ditherer;
  ditherer.reset (16);

  float     samples[] = { 0.5f, -0.5f, 0.25f, -0.25f };
  const int num_samples = 4;

  // Store original samples for comparison
  float original[4];
  std::copy (samples, samples + num_samples, original);

  ditherer.process (samples, num_samples);

  // Verify samples have been modified but remain within reasonable bounds
  for (int i = 0; i < num_samples; i++)
    {
      EXPECT_NE (
        samples[i], original[i]); // Should be different due to dithering
      EXPECT_TRUE (samples[i] >= -1.0f && samples[i] <= 1.0f); // Within bounds
    }
}

TEST (DithererTest, ConsistentProcessing)
{
  zrythm::dsp::Ditherer ditherer;
  ditherer.reset (16);

  // Process same value multiple times
  float sample1[] = { 0.3f };
  float sample2[] = { 0.3f };

  ditherer.process (sample1, 1);
  ditherer.reset (16);
  ditherer.process (sample2, 1);

  // Results should be different due to random dithering
  EXPECT_NE (sample1[0], sample2[0]);
}

TEST (DithererTest, DifferentBitDepths)
{
  zrythm::dsp::Ditherer ditherer;
  float                 sample = 0.5f;

  // Test with different bit depths
  ditherer.reset (8);
  float sample8 = sample;
  ditherer.process (&sample8, 1);

  ditherer.reset (16);
  float sample16 = sample;
  ditherer.process (&sample16, 1);

  ditherer.reset (24);
  float sample24 = sample;
  ditherer.process (&sample24, 1);

  // Higher bit depth should result in less aggressive dithering
  float diff8 = std::abs (sample - sample8);
  float diff16 = std::abs (sample - sample16);
  float diff24 = std::abs (sample - sample24);

  EXPECT_GT (diff8, diff16);
  EXPECT_GT (diff16, diff24);
}
