// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <cstdlib>

#include "utils/float_ranges.h"

#include <benchmark/benchmark.h>

using namespace zrythm::utils::float_ranges;

namespace
{

template <size_t N>
std::array<float, N>
make_signal (float value)
{
  std::array<float, N> buf{};
  buf.fill (value);
  return buf;
}

} // namespace

static void
BM_Fill (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> buf (size);
  for (auto _ : state)
    {
      fill (buf, 0.5f);
      benchmark::DoNotOptimize (buf.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_Fill)->Range (64, 8192)->Complexity ();

static void
BM_Clip (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> buf (size, 2.0f);
  for (auto _ : state)
    {
      clip (buf, -1.0f, 1.0f);
      benchmark::DoNotOptimize (buf.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_Clip)->Range (64, 8192)->Complexity ();

static void
BM_Copy (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> src (size, 0.5f);
  std::vector<float> dest (size);
  for (auto _ : state)
    {
      copy (dest, src);
      benchmark::DoNotOptimize (dest.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_Copy)->Range (64, 8192)->Complexity ();

static void
BM_MulK2 (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> buf (size, 0.5f);
  for (auto _ : state)
    {
      mul_k2 (buf, 0.99f);
      benchmark::DoNotOptimize (buf.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_MulK2)->Range (64, 8192)->Complexity ();

static void
BM_AbsMax (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> buf (size);
  for (size_t i = 0; i < size; ++i)
    buf[i] = static_cast<float> (i % 256) / 256.f - 0.5f;
  for (auto _ : state)
    {
      auto peak = abs_max (buf);
      benchmark::DoNotOptimize (peak);
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_AbsMax)->Range (64, 8192)->Complexity ();

static void
BM_Add2 (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> dest (size, 0.5f);
  std::vector<float> src (size, 0.3f);
  for (auto _ : state)
    {
      add2 (dest, src);
      benchmark::DoNotOptimize (dest.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_Add2)->Range (64, 8192)->Complexity ();

static void
BM_Product (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> dest (size);
  std::vector<float> src (size, 0.5f);
  for (auto _ : state)
    {
      product (dest, src, 0.8f);
      benchmark::DoNotOptimize (dest.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_Product)->Range (64, 8192)->Complexity ();

static void
BM_MixProduct (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> dest (size, 0.5f);
  std::vector<float> src (size, 0.3f);
  for (auto _ : state)
    {
      mix_product (dest, src, 0.8f);
      benchmark::DoNotOptimize (dest.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_MixProduct)->Range (64, 8192)->Complexity ();

static void
BM_Reverse (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> src (size);
  for (size_t i = 0; i < size; ++i)
    src[i] = static_cast<float> (i);
  std::vector<float> dest (size);
  for (auto _ : state)
    {
      zrythm::utils::float_ranges::reverse (dest, src);
      benchmark::DoNotOptimize (dest.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_Reverse)->Range (64, 8192)->Complexity ();

static void
BM_Normalize (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> src (size);
  for (size_t i = 0; i < size; ++i)
    src[i] = static_cast<float> (i % 128) / 128.f;
  std::vector<float> dest (size);
  for (auto _ : state)
    {
      normalize (dest, src);
      benchmark::DoNotOptimize (dest.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_Normalize)->Range (64, 8192)->Complexity ();

static void
BM_MakeMono (benchmark::State &state)
{
  const auto         size = static_cast<size_t> (state.range (0));
  std::vector<float> l (size, 0.5f);
  std::vector<float> r (size, 0.3f);
  for (auto _ : state)
    {
      make_mono (l, r, true);
      benchmark::DoNotOptimize (l.data ());
    }
  state.SetComplexityN (size);
}
BENCHMARK (BM_MakeMono)->Range (64, 8192)->Complexity ();

BENCHMARK_MAIN ();
