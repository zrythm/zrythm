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

#include "Processor.h"

#include "Convolver.h"
#include "DecibelScaling.h"
#include "Envelope.h"
#include "IRCalculation.h"
#include "IRAgent.h"
#include "Parameters.h"
#include "Processor.h"

#include <algorithm>


class FloatBufferSource : public AudioSource
{
public:
  explicit FloatBufferSource(const FloatBuffer::Ptr& sourceBuffer) :
    juce::AudioSource(),
    _sourceBuffer(sourceBuffer),
    _pos(0)
  {
  }

  virtual void prepareToPlay(int /*samplesPerBlockExpected*/, double /*sampleRate*/)
  {
    _pos = 0;
  }

  virtual void releaseResources()
  {
    _pos = 0;
  }

  virtual void getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
  {
    AudioSampleBuffer* destBuffer = bufferToFill.buffer;
    const int len = std::min(bufferToFill.numSamples, static_cast<int>(_sourceBuffer->getSize()-_pos));
    if (destBuffer)
    {
      for (int channel=0; channel<destBuffer->getNumChannels(); ++channel)
      {
        if (channel == 0 && _sourceBuffer)
        {
          destBuffer->copyFrom(channel, bufferToFill.startSample, _sourceBuffer->data()+_pos, len);
          if (len < bufferToFill.numSamples)
          {
            const int startClear = bufferToFill.startSample + len;
            const int lenClear = bufferToFill.numSamples - len;
            destBuffer->clear(startClear, lenClear);
          }
        }
        else
        {
          destBuffer->clear(channel, bufferToFill.startSample, len);
        }
      }
    }
    _pos += len;
  }

private:
  const FloatBuffer::Ptr& _sourceBuffer;
  size_t _pos;

  FloatBufferSource(const FloatBufferSource&);
  FloatBufferSource& operator=(const FloatBufferSource&);
};


// ===================================================================


IRCalculation::IRCalculation(Processor& processor) :
  juce::Thread("IRCalculation"),
  _processor(processor)
{
  startThread();
}


IRCalculation::~IRCalculation()
{
  stopThread(-1);
}


void IRCalculation::run()
{
  if (threadShouldExit())
  {
    return;
  }

  // Initiate fade out
  IRAgentContainer agents = _processor.getAgents();
  for (size_t i=0; i<agents.size(); ++i)
  {
    agents[i]->fadeOut();
  }
  
  // Import the files
  std::vector<FloatBuffer::Ptr> buffers(agents.size(), nullptr);
  std::vector<double> fileSampleRates(agents.size(), 0.0);
  for (size_t i=0; i<agents.size(); ++i)
  {
    const juce::File file = agents[i]->getFile();
    if (file.exists())
    {
      double sampleRate;
      FloatBuffer::Ptr buffer = importAudioFile(file, agents[i]->getFileChannel(), sampleRate);
      if (!buffer || sampleRate < 0.0001 || threadShouldExit())
      {
        return;
      }
      buffers[i] = buffer;
      fileSampleRates[i] = sampleRate;
    }
  }

  // Sample rate
  const double convolverSampleRate = _processor.getSampleRate();
  const double stretch = _processor.getStretch();
  const double stretchSampleRate = convolverSampleRate * stretch;
  for (size_t i=0; i<agents.size(); ++i)
  { 
    if (buffers[i] != nullptr && fileSampleRates[i] > 0.00001)
    {
      FloatBuffer::Ptr resampled = changeSampleRate(buffers[i], fileSampleRates[i], stretchSampleRate);
      if (!resampled || threadShouldExit())
      {
        return;
      }
      buffers[i] = resampled;
    }
  }

  // Unify buffer size
  unifyBufferSize(buffers);
  if (threadShouldExit())
  { 
    return;
  }

  // Crop begin/end
  buffers = cropBuffers(buffers, _processor.getIRBegin(), _processor.getIREnd());
  if (threadShouldExit())
  {
    return;
  }

  // Calculate auto gain (should be done before applying the envelope!)
  const float autoGain = calculateAutoGain(buffers);
  if (threadShouldExit())
  {
    return;
  }

  // Envelope
  const double attackLength = _processor.getAttackLength();
  const double attackShape = _processor.getAttackShape();
  const double decayShape = _processor.getDecayShape();
  for (size_t i=0; i<buffers.size(); ++i)
  {
    if (buffers[i] != nullptr)
    {
      ApplyEnvelope(buffers[i]->data(), buffers[i]->getSize(), attackLength, attackShape, decayShape);
    }
    if (threadShouldExit())
    {
      return;
    }
  }  

  // Reverse
  if (_processor.getReverse())
  {
    for (size_t i=0; i<buffers.size(); ++i)
    {
      if (buffers[i] != nullptr)
      {
        reverseBuffer(buffers[i]);
      }
      if (threadShouldExit())
      {
        return;
      }
    }
  }

  // Predelay
  const double predelayMs = _processor.getPredelayMs();
  const size_t predelaySamples = static_cast<size_t>((convolverSampleRate / 1000.0) * predelayMs);
  if (predelaySamples > 0)
  {
    for (size_t i=0; i<buffers.size(); ++i)
    {
      if (buffers[i] != nullptr)
      {
        FloatBuffer::Ptr buffer = new FloatBuffer(buffers[i]->getSize() + predelaySamples);
        ::memset(buffer->data(), 0, predelaySamples * sizeof(float));
        ::memcpy(buffer->data()+predelaySamples, buffers[i]->data(), buffers[i]->getSize() * sizeof(float));
        buffers[i] = buffer;
      }      
    }
  }
  
  // Update convolvers
  const size_t headBlockSize = _processor.getConvolverHeadBlockSize();
  const size_t tailBlockSize = _processor.getConvolverTailBlockSize();
  _processor.setParameter(Parameters::AutoGainDecibels, DecibelScaling::Gain2Db(autoGain));
  for (size_t i=0; i<agents.size(); ++i)
  {
    juce::ScopedPointer<Convolver> convolver(new Convolver());
    if (buffers[i] != nullptr && buffers[i]->getSize() > 0)
    {        
      convolver = new Convolver();
      const bool successInit = convolver->init(headBlockSize, tailBlockSize, buffers[i]->data(), buffers[i]->getSize());
      if (!successInit || threadShouldExit())
      {
        return;
      }
    }

    while (!agents[i]->waitForFadeOut(1))
    {
      if (threadShouldExit())
      {
        return;
      }
    }
    agents[i]->resetIR(buffers[i], convolver.release());
    agents[i]->fadeIn();
  }
}



FloatBuffer::Ptr IRCalculation::importAudioFile(const File& file, size_t fileChannel, double& fileSampleRate) const
{
  fileSampleRate = 0.0;

  if (! file.exists())
  {
    return FloatBuffer::Ptr();
  }

  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  ScopedPointer<AudioFormatReader> audioFormatReader(formatManager.createReaderFor(file));
  if (!audioFormatReader)
  {
    return FloatBuffer::Ptr();
  }

  const int fileChannels = audioFormatReader->numChannels;
  const size_t fileLen = static_cast<size_t>(audioFormatReader->lengthInSamples);
  if (static_cast<int>(fileChannel) >= fileChannels)
  {
    return FloatBuffer::Ptr();
  }
  FloatBuffer::Ptr buffer(new FloatBuffer(fileLen));

  juce::AudioFormatReaderSource audioFormatReaderSource(audioFormatReader, false);
  size_t pos = 0;
  juce::AudioSampleBuffer importBuffer(fileChannels, 8192);
  while (pos < fileLen)
  {
    if (threadShouldExit())
    {
      return FloatBuffer::Ptr();
    }
    const int loading = std::min(importBuffer.getNumSamples(), static_cast<int>(fileLen-pos));
    juce::AudioSourceChannelInfo info;
    info.buffer = &importBuffer;
    info.startSample = 0;
    info.numSamples = loading;
    audioFormatReaderSource.getNextAudioBlock(info);
    ::memcpy(buffer->data()+pos, importBuffer.getReadPointer(fileChannel), static_cast<size_t>(loading) * sizeof(float));
    pos += static_cast<size_t>(loading);
  }

  fileSampleRate = audioFormatReader->sampleRate;
  return buffer;
}

void IRCalculation::reverseBuffer(FloatBuffer::Ptr& buffer) const
{
  if (buffer)
  {
    const size_t bufferSize = buffer->getSize();
    if (bufferSize > 0)
    {
      float* a = buffer->data();
      float* b = a + (bufferSize - 1);
      while (a < b)
      {
        std::swap(*a, *b);
        ++a;
        --b;
      }
    }
  }
}


std::vector<FloatBuffer::Ptr> IRCalculation::cropBuffers(const std::vector<FloatBuffer::Ptr>& buffers, double irBegin, double irEnd) const
{
  const double Epsilon = 0.00001;
  irBegin = std::min(1.0, std::max(0.0, irBegin));
  irEnd = std::min(1.0, std::max(0.0, irEnd));
  if (irBegin < Epsilon && irEnd > 1.0-Epsilon)
  {
    return buffers;
  }
  std::vector<FloatBuffer::Ptr> croppedBuffers;
  for (size_t i=0; i<buffers.size(); ++i)
  {
    if (threadShouldExit())
    {
      croppedBuffers.clear();
      return croppedBuffers;
    }
    FloatBuffer::Ptr cropped;
    FloatBuffer::Ptr buffer = buffers[i];
    if (buffer)
    {
      const size_t bufferSize = buffer->getSize();
      const size_t begin = static_cast<size_t>(irBegin * static_cast<double>(bufferSize));
      const size_t end = static_cast<size_t>(irEnd * static_cast<double>(bufferSize));      
      if (begin < end)
      {
        const size_t croppedSize = end - begin;
        cropped = new FloatBuffer(croppedSize);
        ::memcpy(cropped->data(), buffer->data()+begin, croppedSize * sizeof(float));
      }
    }
    croppedBuffers.push_back(cropped);
  }
  return croppedBuffers;
}


FloatBuffer::Ptr IRCalculation::changeSampleRate(const FloatBuffer::Ptr& inputBuffer, double inputSampleRate, double outputSampleRate) const
{
  if (!inputBuffer)
  {
    return FloatBuffer::Ptr();
  }

  if (::fabs(outputSampleRate-inputSampleRate) < 0.0000001)
  {
    return inputBuffer;
  }

  jassert(inputSampleRate >= 1.0);
  jassert(outputSampleRate >= 1.0);

  const double samplesInPerOutputSample = inputSampleRate / outputSampleRate;
  const int inputSampleCount = inputBuffer->getSize();
  const int outputSampleCount = static_cast<int>(::ceil(static_cast<double>(inputSampleCount) / samplesInPerOutputSample));
  const int blockSize = 8192;

  FloatBufferSource inputSource(inputBuffer);
  ResamplingAudioSource resamplingSource(&inputSource, false, 1);
  resamplingSource.setResamplingRatio(samplesInPerOutputSample);
  resamplingSource.prepareToPlay(blockSize, outputSampleRate);

  FloatBuffer::Ptr outputBuffer(new FloatBuffer(outputSampleCount));
  juce::AudioSampleBuffer blockBuffer(1, blockSize);
  int processed = 0;
  while (processed < outputSampleCount)
  {
    if (threadShouldExit())
    {
      return FloatBuffer::Ptr();
    }

    const int remaining = outputSampleCount - processed;
    const int processing = std::min(blockSize, remaining);

    juce::AudioSourceChannelInfo info;
    info.buffer = &blockBuffer;
    info.startSample = 0;
    info.numSamples = processing;
    resamplingSource.getNextAudioBlock(info);      
    ::memcpy(outputBuffer->data()+static_cast<size_t>(processed), blockBuffer.getReadPointer(0), processing * sizeof(float));
    processed += processing;
  }

  resamplingSource.releaseResources();

  return outputBuffer;
}


void IRCalculation::unifyBufferSize(std::vector<FloatBuffer::Ptr>& buffers) const
{
  size_t bufferSize = 0;
  for (size_t i=0; i<buffers.size(); ++i)
  {
    if (buffers[i] != nullptr)
    {
      bufferSize = std::max(bufferSize, buffers[i]->getSize());
    }
  }
  for (size_t i=0; i<buffers.size(); ++i)
  {
    if (threadShouldExit())
    {
      return;
    }
    if (buffers[i] && buffers[i]->getSize() < bufferSize)
    {
      FloatBuffer::Ptr buffer(new FloatBuffer(bufferSize));
      const size_t copySize = buffers[i]->getSize();
      const size_t padSize = bufferSize - copySize;
      ::memcpy(buffer->data(), buffers[i]->data(), copySize * sizeof(float));
      ::memset(buffer->data() + copySize, 0, padSize * sizeof(float));
      buffers[i] = buffer;
    }
  }
}


float IRCalculation::calculateAutoGain(const std::vector<FloatBuffer::Ptr>& buffers) const
{
  double autoGain = 1.0;
  for (size_t buffer=0; buffer<buffers.size(); ++buffer)
  {
    if (threadShouldExit())
    {
      return -1.0f;
    }
    if (buffers[buffer] != nullptr)
    {
      const float* data = buffers[buffer]->data();
      const size_t len = buffers[buffer]->getSize();
      double sum = 0.0;
      for (size_t i=0; i<len; ++i)
      {
        const double val = static_cast<double>(data[i]);
        sum += val * val;
      }
      const double x = 1.0 / std::sqrt(sum);
      autoGain = std::min(autoGain, x);
    }
  }
  return static_cast<float>(autoGain);
}
