// ==================================================================================
// Copyright (c) 2012 HiFi-LoFi
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ==================================================================================

#include <algorithm>
#include <cstring>
#include <numeric>
#include <vector>

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../FFTConvolver.h"
#include "../TwoStageFFTConvolver.h"
#include "../Utilities.h"


template<typename T>
void SimpleConvolve(const T* input, size_t inLen, const T* ir, size_t irLen, T* output)
{
  if (irLen > inLen)
  {
    SimpleConvolve(ir, irLen, input, inLen, output);
    return;
  }
  
  ::memset(output, 0, (inLen+irLen-1) * sizeof(T));
  
  for (size_t n=0; n<irLen; ++n)
  {
    for (size_t m=0; m<=n; ++m)
    {
      output[n] += ir[m] * input[n-m];
    }
  }
  
  for (size_t n=irLen; n<inLen; ++n)
  {
    for (size_t m=0; m<irLen; ++m)
    {
      output[n] += ir[m] * input[n-m];
    }
  }
  
  for (size_t n=inLen; n<inLen+irLen-1; ++n)
  {
    for (size_t m=n-inLen+1; m<irLen; ++m)
    {
      output[n] += ir[m] * input[n-m];
    }
  }
}


static bool TestConvolver(size_t inputSize,
                          size_t irSize,
                          size_t blockSizeMin,
                          size_t blockSizeMax,
                          size_t blockSizeConvolver,
                          bool refCheck)
{
  // Prepare input and IR
  std::vector<fftconvolver::Sample> in(inputSize);
  for (size_t i=0; i<inputSize; ++i)
  {
    in[i] = 0.1f * static_cast<fftconvolver::Sample>(i+1);
  }

  std::vector<fftconvolver::Sample> ir(irSize);
  for (size_t i=0; i<irSize; ++i)
  {
    ir[i] = 0.1f * static_cast<fftconvolver::Sample>(i+1);
  }
  
  // Simple convolver
  std::vector<fftconvolver::Sample> outSimple(in.size() + ir.size() - 1, fftconvolver::Sample(0.0));
  if (refCheck)
  {
    SimpleConvolve(&in[0], in.size(), &ir[0], ir.size(), &outSimple[0]);
  }
  
  // FFT convolver
  std::vector<fftconvolver::Sample> out(in.size() + ir.size() - 1, fftconvolver::Sample(0.0));
  {
    fftconvolver::FFTConvolver convolver;
    convolver.init(blockSizeConvolver, &ir[0], ir.size());
    std::vector<fftconvolver::Sample> inBuf(blockSizeMax);
    size_t processedOut = 0;
    size_t processedIn = 0;
    while (processedOut < out.size())
    {
      const size_t blockSize = blockSizeMin + (static_cast<size_t>(rand()) % (1+(blockSizeMax-blockSizeMin))); 
      
      const size_t remainingOut = out.size() - processedOut;
      const size_t remainingIn = in.size() - processedIn;
      
      const size_t processingOut = std::min(remainingOut, blockSize);
      const size_t processingIn = std::min(remainingIn, blockSize);
      
      memset(&inBuf[0], 0, inBuf.size() * sizeof(fftconvolver::Sample));
      if (processingIn > 0)
      {
        memcpy(&inBuf[0], &in[processedIn], processingIn * sizeof(fftconvolver::Sample));
      }
      
      convolver.process(&inBuf[0], &out[processedOut], processingOut);
      
      processedOut += processingOut;
      processedIn += processingIn;
    }
  }
  
  if (refCheck)
  {
    size_t diffSamples = 0;
    const double absTolerance = 0.001 * static_cast<double>(ir.size());
    const double relTolerance = 0.0001 * ::log(static_cast<double>(ir.size()));
    for (size_t i=0; i<outSimple.size(); ++i)
    {      
      const double a = static_cast<double>(out[i]);
      const double b = static_cast<double>(outSimple[i]);
      if (::fabs(a) > 1.0 && ::fabs(b) > 1.0)
      {
        const double absError = ::fabs(a-b);
        const double relError = absError / b;
        if (relError > relTolerance && absError > absTolerance)
        {
          ++diffSamples;
        }
      }
    }
    printf("Correctness Test (input %d, IR %d, blocksize %d-%d) => %s\n", static_cast<int>(inputSize), static_cast<int>(irSize), static_cast<int>(blockSizeMin), static_cast<int>(blockSizeMax), (diffSamples == 0) ? "[OK]" : "[FAILED]");
    return (diffSamples == 0);
  }
  else
  {
    printf("Performance Test (input %d, IR %d, blocksize %d-%d) => Completed\n", static_cast<int>(inputSize), static_cast<int>(irSize), static_cast<int>(blockSizeMin), static_cast<int>(blockSizeMax));
    return true;
  }
}


static bool TestTwoStageConvolver(size_t inputSize,
                                  size_t irSize,
                                  size_t blockSizeMin,
                                  size_t blockSizeMax,
                                  size_t blockSizeHead,
                                  size_t blockSizeTail,
                                  bool refCheck)
{
  // Prepare input and IR
  std::vector<fftconvolver::Sample> in(inputSize);
  for (size_t i=0; i<inputSize; ++i)
  {
    in[i] = 0.1f * static_cast<fftconvolver::Sample>(i+1);
  }

  std::vector<fftconvolver::Sample> ir(irSize);
  for (size_t i=0; i<irSize; ++i)
  {
    ir[i] = 0.1f * static_cast<fftconvolver::Sample>(i+1);
  }
  
  // Simple convolver
  std::vector<fftconvolver::Sample> outSimple(in.size() + ir.size() - 1, fftconvolver::Sample(0.0));
  if (refCheck)
  {
    SimpleConvolve(&in[0], in.size(), &ir[0], ir.size(), &outSimple[0]);
  }
  
  // FFT convolver
  std::vector<fftconvolver::Sample> out(in.size() + ir.size() - 1, fftconvolver::Sample(0.0));
  {
    fftconvolver::TwoStageFFTConvolver convolver;
    convolver.init(blockSizeHead, blockSizeTail, &ir[0], ir.size());
    std::vector<fftconvolver::Sample> inBuf(blockSizeMax);
    size_t processedOut = 0;
    size_t processedIn = 0;
    while (processedOut < out.size())
    {
      const size_t blockSize = blockSizeMin + (static_cast<size_t>(rand()) % (1+(blockSizeMax-blockSizeMin))); 
      
      const size_t remainingOut = out.size() - processedOut;
      const size_t remainingIn = in.size() - processedIn;
      
      const size_t processingOut = std::min(remainingOut, blockSize);
      const size_t processingIn = std::min(remainingIn, blockSize);
      
      memset(&inBuf[0], 0, inBuf.size() * sizeof(fftconvolver::Sample));
      if (processingIn > 0)
      {
        memcpy(&inBuf[0], &in[processedIn], processingIn * sizeof(fftconvolver::Sample));
      }
      
      convolver.process(&inBuf[0], &out[processedOut], processingOut);
      
      processedOut += processingOut;
      processedIn += processingIn;
    }
  }
  
  if (refCheck)
  {
    size_t diffSamples = 0;
    const double absTolerance = 0.001 * static_cast<double>(ir.size());
    const double relTolerance = 0.0001 * ::log(static_cast<double>(ir.size()));   
    for (size_t i=0; i<outSimple.size(); ++i)
    {      
      const double a = static_cast<double>(out[i]);
      const double b = static_cast<double>(outSimple[i]);
      if (::fabs(a) > 1.0 && ::fabs(b) > 1.0)
      {
        const double absError = ::fabs(a-b);
        const double relError = absError / b;
        if (relError > relTolerance && absError > absTolerance)
        {
          ++diffSamples;
        }
      }
    }
    printf("Correctness Test (2-stage, input %d, IR %d, blocksize %d-%d) => %s\n", static_cast<int>(inputSize), static_cast<int>(irSize), static_cast<int>(blockSizeMin), static_cast<int>(blockSizeMax), (diffSamples == 0) ? "[OK]" : "[FAILED]");
    return (diffSamples == 0);
  }
  else
  {
    printf("Performance Test (2-stage, input %d, IR %d, blocksize %d-%d) => Completed\n", static_cast<int>(inputSize), static_cast<int>(irSize), static_cast<int>(blockSizeMin), static_cast<int>(blockSizeMax));
    return true;
  }
}


#define TEST_CORRECTNESS
//#define TEST_PERFORMANCE

#define TEST_FFTCONVOLVER
#define TEST_TWOSTAGEFFTCONVOLVER


int main()
{ 
#if defined(TEST_CORRECTNESS) && defined(TEST_FFTCONVOLVER)
  TestConvolver(1, 1, 1, 1, 1, true);
  TestConvolver(2, 2, 2, 2, 2, true);
  TestConvolver(3, 3, 3, 3, 3, true);
  
  TestConvolver(3, 2, 2, 2, 2, true);
  TestConvolver(4, 2, 2, 2, 2, true);
  TestConvolver(4, 3, 2, 2, 2, true);
  TestConvolver(9, 4, 3, 3, 2, true);
  TestConvolver(171, 7, 5, 5, 5, true);
  TestConvolver(1979, 17, 7, 7, 5, true);
  TestConvolver(100, 10, 3, 5, 5, true);
  TestConvolver(123, 45, 12, 34, 34, true);
  
  TestConvolver(2, 3, 2, 2, 2, true);
  TestConvolver(2, 4, 2, 2, 2, true);
  TestConvolver(3, 4, 2, 2, 2, true);
  TestConvolver(4, 9, 3, 3, 3, true);
  TestConvolver(7, 171, 5, 5, 5, true);
  TestConvolver(17, 1979, 7, 7, 7, true);
  TestConvolver(10, 100, 3, 5, 5, true);
  TestConvolver(45, 123, 12, 34, 34, true);
  
  TestConvolver(100000, 1234, 100,  128,  128, true);
  TestConvolver(100000, 1234, 100,  256,  256, true);
  TestConvolver(100000, 1234, 100,  512,  512, true);
  TestConvolver(100000, 1234, 100, 1024, 1024, true);
  TestConvolver(100000, 1234, 100, 2048, 2048, true);

  TestConvolver(100000, 4321, 100,  128,  128, true);
  TestConvolver(100000, 4321, 100,  256,  256, true);
  TestConvolver(100000, 4321, 100,  512,  512, true);
  TestConvolver(100000, 4321, 100, 1024, 1024, true);
  TestConvolver(100000, 4321, 100, 2048, 2048, true);
#endif
  

#if defined(TEST_PERFORMANCE) && defined(TEST_FFTCONVOLVER)
  TestConvolver(3*60*44100, 20*44100, 50, 100, 1024, false);
#endif
  
#if defined(TEST_CORRECTNESS) && defined(TEST_TWOSTAGEFFTCONVOLVER)
  TestTwoStageConvolver(1, 1, 1, 1, 1, 1, true);
  TestTwoStageConvolver(2, 2, 2, 2, 2, 2, true);
  TestTwoStageConvolver(3, 3, 3, 3, 3, 3, true);

  TestTwoStageConvolver(3, 2, 2, 2, 2, 4, true);
  TestTwoStageConvolver(4, 2, 2, 2, 2, 4, true);
  TestTwoStageConvolver(4, 3, 2, 2, 2, 4, true);
  TestTwoStageConvolver(9, 4, 3, 3, 2, 4, true);
  TestTwoStageConvolver(171, 7, 5, 5, 5, 10,true);
  TestTwoStageConvolver(1979, 17, 7, 7, 5, 10, true);
  TestTwoStageConvolver(100, 10, 3, 5, 5, 10, true);
  TestTwoStageConvolver(123, 45, 12, 34, 34, 68, true);

  TestTwoStageConvolver(2, 3, 2, 2, 1, 2, true);
  TestTwoStageConvolver(2, 4, 2, 2, 1, 2, true);
  TestTwoStageConvolver(3, 4, 2, 2, 1, 2, true);
  TestTwoStageConvolver(4, 9, 3, 3, 2, 4, true);
  TestTwoStageConvolver(7, 171, 5, 5, 2, 16, true);
  TestTwoStageConvolver(17, 1979, 7, 7, 4, 16, true);
  TestTwoStageConvolver(10, 100, 3, 5, 1, 4, true);
  TestTwoStageConvolver(45, 123, 12, 34, 4, 32, true);

  TestTwoStageConvolver(100000, 1234, 100,  128,  128, 4096, true);
  TestTwoStageConvolver(100000, 1234, 100,  256,  256, 4096, true);
  TestTwoStageConvolver(100000, 1234, 100,  512,  512, 4096, true);
  TestTwoStageConvolver(100000, 1234, 100, 1024, 1024, 4096, true);
  TestTwoStageConvolver(100000, 1234, 100, 2048, 2048, 4096, true);

  TestTwoStageConvolver(100000, 4321, 100,  128,  128, 4096, true);
  TestTwoStageConvolver(100000, 4321, 100,  256,  256, 4096, true);
  TestTwoStageConvolver(100000, 4321, 100,  512,  512, 4096, true);
  TestTwoStageConvolver(100000, 4321, 100, 1024, 1024, 4096, true);
  TestTwoStageConvolver(100000, 4321, 100, 2048, 2048, 4096, true);
#endif


#if defined(TEST_PERFORMANCE) && defined(TEST_TWOSTAGEFFTCONVOLVER)
  TestTwoStageConvolver(3*60*44100, 20*44100, 50, 100, 100, 2*8192, false);
#endif
  
  return 0;
}
