#include "utils/audio.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::utils::audio;

TEST (AudioTest, BitDepthConversion)
{
  EXPECT_EQ (bit_depth_enum_to_int (BitDepth::BIT_DEPTH_16), 16);
  EXPECT_EQ (bit_depth_enum_to_int (BitDepth::BIT_DEPTH_24), 24);
  EXPECT_EQ (bit_depth_enum_to_int (BitDepth::BIT_DEPTH_32), 32);

  EXPECT_EQ (bit_depth_int_to_enum (16), BitDepth::BIT_DEPTH_16);
  EXPECT_EQ (bit_depth_int_to_enum (24), BitDepth::BIT_DEPTH_24);
  EXPECT_EQ (bit_depth_int_to_enum (32), BitDepth::BIT_DEPTH_32);
}

TEST (AudioTest, GetNumFrames)
{
  auto num_frames = get_num_frames (TEST_WAV_FILE_PATH);
  EXPECT_GT (num_frames, 0);
}

TEST (AudioTest, FrameComparison)
{
  float buf1[1024];
  float buf2[1024];

  // Test equal buffers
  std::fill_n (buf1, 1024, 0.5f);
  std::fill_n (buf2, 1024, 0.5f);
  EXPECT_TRUE (frames_equal (buf1, buf2, 1024, 0.0001f));

  // Test different buffers
  buf2[500] = 0.6f;
  EXPECT_FALSE (frames_equal (buf1, buf2, 1024, 0.0001f));
}

TEST (AudioTest, AudioFileComparison)
{
  EXPECT_TRUE (
    audio_files_equal (TEST_WAV_FILE_PATH, TEST_WAV_FILE_PATH, 0, 0.0001f));
}

TEST (AudioTest, FramesEmpty)
{
  float buf[1024] = { 0.f };
  EXPECT_TRUE (frames_empty (buf, 1024));

  buf[500] = 0.1f;
  EXPECT_FALSE (frames_empty (buf, 1024));
}

// TODO
#if 0
TEST (AudioTest, BpmDetection)
{
  std::vector<float> candidates;
  float              bpm = detect_bpm (nullptr, 0, 44100, candidates);
  EXPECT_GE (bpm, 0.f);
}
#endif

TEST (AudioTest, AudioFileSilence)
{
  EXPECT_FALSE (audio_file_is_silent (TEST_WAV_FILE_PATH));
}

TEST (AudioTest, NumCores)
{
  int cores = get_num_cores ();
  EXPECT_GT (cores, 0);
}
