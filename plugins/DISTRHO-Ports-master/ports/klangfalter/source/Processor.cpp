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

#include "IRAgent.h"
#include "IRCalculation.h"
#include "Parameters.h"
#include "Persistence.h"
#include "Settings.h"
#include "UI/KlangFalterEditor.h"

#include <algorithm>


//==============================================================================
Processor::Processor() :
  AudioProcessor(),
  ChangeNotifier(),
  _wetBuffer(1, 0),
  _convolutionBuffer(),
  _parameterSet(),
  _levelMeasurementsDry(2),
  _levelMeasurementsWet(2),
  _levelMeasurementsOut(2),
  _settings(),
  _convolverMutex(),
  _agents(),
  _stretch(1.0),
  _reverse(false),
  _convolverHeadBlockSize(0),
  _convolverTailBlockSize(0),
  _irBegin(0.0),
  _irEnd(1.0),
  _predelayMs(0.0),
  _attackLength(0.0),
  _attackShape(0.0),
  _decayShape(0.0),
  _stereoWidth(),
  _dryOn(Parameters::DryOn.getDefaultValue() ? 1.0f : 0.0f),
  _wetOn(Parameters::WetOn.getDefaultValue() ? 1.0f : 0.0f),
  _dryGain(DecibelScaling::Db2Gain(Parameters::DryDecibels.getDefaultValue())),
  _wetGain(DecibelScaling::Db2Gain(Parameters::WetDecibels.getDefaultValue())),
  _beatsPerMinute(0.0f),
  _irCalculationMutex(),
  _irCalculation()
{ 
  _parameterSet.registerParameter(Parameters::WetOn);
  _parameterSet.registerParameter(Parameters::WetDecibels);
  _parameterSet.registerParameter(Parameters::DryOn);
  _parameterSet.registerParameter(Parameters::DryDecibels);
  _parameterSet.registerParameter(Parameters::EqLowType);
  _parameterSet.registerParameter(Parameters::EqLowCutFreq);
  _parameterSet.registerParameter(Parameters::EqLowShelfFreq);
  _parameterSet.registerParameter(Parameters::EqLowShelfDecibels);
  _parameterSet.registerParameter(Parameters::EqHighType);
  _parameterSet.registerParameter(Parameters::EqHighCutFreq);
  _parameterSet.registerParameter(Parameters::EqHighShelfFreq);
  _parameterSet.registerParameter(Parameters::EqHighShelfDecibels);
  _parameterSet.registerParameter(Parameters::StereoWidth);
  _parameterSet.registerParameter(Parameters::AutoGainOn);
  _parameterSet.registerParameter(Parameters::AutoGainDecibels);

  _agents.push_back(new IRAgent(*this, 0, 0));
  _agents.push_back(new IRAgent(*this, 0, 1));
  _agents.push_back(new IRAgent(*this, 1, 0));
  _agents.push_back(new IRAgent(*this, 1, 1));
}


Processor::~Processor()
{
  Processor::releaseResources();

  for (size_t i=0; i<_agents.size(); ++i)
  {
    delete _agents[i];
  }
  _agents.clear();
}


//==============================================================================
const String Processor::getName() const
{
#ifdef JucePlugin_Name
  return JucePlugin_Name;
#else
  return String("Plugin Test");
#endif
}

int Processor::getNumParameters()
{
  // ignore last parameter (autogain)
  return _parameterSet.getParameterCount() -1;
}

float Processor::getParameter(int index)
{
  return _parameterSet.getNormalizedParameter(index);
}

void Processor::setParameter(int index, float newValue)
{
  if (_parameterSet.setNormalizedParameter(index, newValue))
  {
    notifyAboutChange();
  }
}

const String Processor::getParameterName(int index)
{
  return _parameterSet.getParameterDescriptor(index).getName();
}

const String Processor::getParameterText(int index)
{
  return _parameterSet.getFormattedParameterValue(index);
}

juce::String Processor::getParameterLabel (int index)
{
  return _parameterSet.getParameterDescriptor(index).getUnit();
}

bool Processor::isParameterAutomatable(int index) const
{
  return (_parameterSet.getParameterDescriptor(index).getAutomationStatus() == ParameterDescriptor::Automatable);
}

const String Processor::getInputChannelName(int channelIndex) const
{
  return String (channelIndex + 1);
}

const String Processor::getOutputChannelName(int channelIndex) const
{
  return String (channelIndex + 1);
}

bool Processor::isInputChannelStereoPair(int /*index*/) const
{
  return true;
}

bool Processor::isOutputChannelStereoPair(int /*index*/) const
{
  return true;
}

bool Processor::acceptsMidi() const
{
  return false;
}

bool Processor::producesMidi() const
{
  return false;
}

bool Processor::silenceInProducesSilenceOut() const
{
  return false;
}

double Processor::getTailLengthSeconds() const
{
  return getIRDuration();
}

void Processor::numChannelsChanged()
{
  juce::AudioProcessor::numChannelsChanged();
  notifyAboutChange();
}

int Processor::getNumPrograms()
{
  return 0;
}

int Processor::getCurrentProgram()
{
  return 0;
}

void Processor::setCurrentProgram (int /*index*/)
{
}

const String Processor::getProgramName (int /*index*/)
{
  return String();
}

void Processor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}


//==============================================================================
void Processor::prepareToPlay(double /*sampleRate*/, int samplesPerBlock)
{
  // Play safe to be clean
  releaseResources();

  // Prepare convolvers
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    _convolverHeadBlockSize = 1;
    while (_convolverHeadBlockSize < static_cast<size_t>(samplesPerBlock))
    {
      _convolverHeadBlockSize *= 2;
    }
    _convolverTailBlockSize = std::max(size_t(8192), 2 * _convolverHeadBlockSize);
  }

  // Prepare convolution buffers
  _wetBuffer.setSize(2, samplesPerBlock);
  _convolutionBuffer.resize(samplesPerBlock);

  // Initialize parameters
  _stereoWidth.initializeWidth(getParameter(Parameters::StereoWidth));

  // Initialize IR agents
  for (size_t i=0; i<_agents.size(); ++i)
  {
    _agents[i]->initialize();
  }

  notifyAboutChange();
  updateConvolvers();
}


void Processor::releaseResources()
{
  _wetBuffer.setSize(1, 0, false, true, false);
  _convolutionBuffer.clear();
  _beatsPerMinute.set(0);
  notifyAboutChange();
}


void Processor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& /*midiMessages*/)
{ 
  const int numInputChannels = getTotalNumInputChannels();
  const int numOutputChannels = getTotalNumOutputChannels();
  const size_t samplesToProcess = buffer.getNumSamples();

  // Determine channel data
  float* channelData0 = nullptr;
  float* channelData1 = nullptr;
  if (numInputChannels == 1)
  {    
    channelData0 = buffer.getWritePointer(0);
    channelData1 = buffer.getWritePointer(0);
  }
  else if (numInputChannels == 2)
  {
    channelData0 = buffer.getWritePointer(0);
    channelData1 = buffer.getWritePointer(1);
  }

  // Convolution
  _wetBuffer.clear();
  if (numInputChannels > 0 && numOutputChannels > 0)
  {
    float autoGain;

    if (getParameter(Parameters::AutoGainOn))
      autoGain = DecibelScaling::Db2Gain(getParameter(Parameters::AutoGainDecibels));
    else
      autoGain = 1.0f;

    // Convolve
    IRAgent* irAgent00 = getAgent(0, 0);
    if (irAgent00 && irAgent00->getConvolver() && numInputChannels >= 1 && numOutputChannels >= 1)
    {
      irAgent00->process(channelData0, &_convolutionBuffer[0], samplesToProcess);
      _wetBuffer.addFrom(0, 0, &_convolutionBuffer[0], samplesToProcess, autoGain);
    }

    IRAgent* irAgent01 = getAgent(0, 1);
    if (irAgent01 && irAgent01->getConvolver() && numInputChannels >= 1 && numOutputChannels >= 2)
    {
      irAgent01->process(channelData0, &_convolutionBuffer[0], samplesToProcess);
      _wetBuffer.addFrom(1, 0, &_convolutionBuffer[0], samplesToProcess, autoGain);
    }

    IRAgent* irAgent10 = getAgent(1, 0);
    if (irAgent10 && irAgent10->getConvolver() && numInputChannels >= 2 && numOutputChannels >= 1)
    {      
      irAgent10->process(channelData1, &_convolutionBuffer[0], samplesToProcess);
      _wetBuffer.addFrom(0, 0, &_convolutionBuffer[0], samplesToProcess, autoGain);
    }

    IRAgent* irAgent11 = getAgent(1, 1);
    if (irAgent11 && irAgent11->getConvolver() && numInputChannels >= 2 && numOutputChannels >= 2)
    {
      irAgent11->process(channelData1, &_convolutionBuffer[0], samplesToProcess);
      _wetBuffer.addFrom(1, 0, &_convolutionBuffer[0], samplesToProcess, autoGain);
    }
  }

  // Stereo width
  if (numOutputChannels >= 2)
  {
    _stereoWidth.updateWidth(getParameter(Parameters::StereoWidth));
    _stereoWidth.process(_wetBuffer.getWritePointer(0), _wetBuffer.getWritePointer(1), samplesToProcess);
  }

  // Dry/wet gain
  {
    float dryGain0, dryGain1;
    _dryGain.updateValue(DecibelScaling::Db2Gain(getParameter(Parameters::DryDecibels)));
    _dryGain.getSmoothValues(samplesToProcess, dryGain0, dryGain1);
    buffer.applyGainRamp(0, samplesToProcess, dryGain0, dryGain1);
  }
  {
    float wetGain0, wetGain1;
    _wetGain.updateValue(DecibelScaling::Db2Gain(getParameter(Parameters::WetDecibels)));
    _wetGain.getSmoothValues(samplesToProcess, wetGain0, wetGain1);
    _wetBuffer.applyGainRamp(0, samplesToProcess, wetGain0, wetGain1);
  }

  // Level measurement (dry)
  if (numInputChannels == 1)
  {    
    _levelMeasurementsDry[0].process(samplesToProcess, buffer.getReadPointer(0));
    _levelMeasurementsDry[1].reset();
  }
  else if (numInputChannels == 2)
  {
    _levelMeasurementsDry[0].process(samplesToProcess, buffer.getReadPointer(0));
    _levelMeasurementsDry[1].process(samplesToProcess, buffer.getReadPointer(1));
  }

  // Sum wet to dry signal
  {
    float dryOnGain0, dryOnGain1;
    _dryOn.updateValue(getParameter(Parameters::DryOn) ? 1.0f : 0.0f);
    _dryOn.getSmoothValues(samplesToProcess, dryOnGain0, dryOnGain1);
    buffer.applyGainRamp(0, samplesToProcess, dryOnGain0, dryOnGain1);
  }
  {
    float wetOnGain0, wetOnGain1;
    _wetOn.updateValue(getParameter(Parameters::WetOn) ? 1.0f : 0.0f);
    _wetOn.getSmoothValues(samplesToProcess, wetOnGain0, wetOnGain1);
    if (numOutputChannels > 0)
    {
      buffer.addFromWithRamp(0, 0, _wetBuffer.getReadPointer(0), samplesToProcess, wetOnGain0, wetOnGain1);
    }
    if (numOutputChannels > 1)
    {
      buffer.addFromWithRamp(1, 0, _wetBuffer.getReadPointer(1), samplesToProcess, wetOnGain0, wetOnGain1);
    }
  }

  // Level measurement (wet/out)
  if (numOutputChannels == 1)
  {
    _levelMeasurementsWet[0].process(samplesToProcess, _wetBuffer.getReadPointer(0));
    _levelMeasurementsWet[1].reset();
    _levelMeasurementsOut[0].process(samplesToProcess, buffer.getReadPointer(0));
    _levelMeasurementsOut[1].reset();
  }
  else if (numOutputChannels == 2)
  {
    _levelMeasurementsWet[0].process(samplesToProcess, _wetBuffer.getReadPointer(0));
    _levelMeasurementsWet[1].process(samplesToProcess, _wetBuffer.getReadPointer(1));
    _levelMeasurementsOut[0].process(samplesToProcess, buffer.getReadPointer(0));
    _levelMeasurementsOut[1].process(samplesToProcess, buffer.getReadPointer(1));
  }

  // In case we have more outputs than inputs, we'll clear any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  for (int i=numInputChannels; i<numOutputChannels; ++i)
  {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  // Update beats per minute info
  float beatsPerMinute = 0.0f;
  juce::AudioPlayHead* playHead = getPlayHead();
  if (playHead)
  {
    juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo;
    if (playHead->getCurrentPosition(currentPositionInfo))
    {
      beatsPerMinute = static_cast<float>(currentPositionInfo.bpm);
    }
  }
  if (::fabs(_beatsPerMinute.exchange(beatsPerMinute)-beatsPerMinute) > 0.001f)
  {
    notifyAboutChange();
  }
}

//==============================================================================
bool Processor::hasEditor() const
{
  return true;
}

AudioProcessorEditor* Processor::createEditor()
{
  return new KlangFalterEditor(*this);
}

//==============================================================================
void Processor::getStateInformation (MemoryBlock& destData)
{
  const juce::File irDirectory = _settings.getImpulseResponseDirectory();
  juce::ScopedPointer<juce::XmlElement> element(SaveState(irDirectory, *this));
  if (element)
  {
    copyXmlToBinary(*element, destData);
  }
}


void Processor::setStateInformation (const void* data, int sizeInBytes)
{
  juce::ScopedPointer<juce::XmlElement> element(getXmlFromBinary(data, sizeInBytes));
  if (element)
  {
    const juce::File irDirectory = _settings.getImpulseResponseDirectory();
    LoadState(irDirectory, *element, *this);
  }
}


float Processor::getLevelDry(size_t channel) const
{
  return (channel < _levelMeasurementsDry.size()) ? _levelMeasurementsDry[channel].getLevel() : 0.0f;
}


float Processor::getLevelWet(size_t channel) const
{
  return (channel < _levelMeasurementsWet.size()) ? _levelMeasurementsWet[channel].getLevel() : 0.0f;
}


float Processor::getLevelOut(size_t channel) const
{
  return (channel < _levelMeasurementsOut.size()) ? _levelMeasurementsOut[channel].getLevel() : 0.0f;
}


Settings& Processor::getSettings()
{
  return _settings;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new Processor();
}


// =============================================================================


IRAgent* Processor::getAgent(size_t inputChannel, size_t outputChannel) const
{
  for (size_t i=0; i<_agents.size(); ++i)
  {
    IRAgent* agent = _agents[i];
    if (agent->getInputChannel() == inputChannel && agent->getOutputChannel() == outputChannel)
    {
      return agent;
    }
  }
  return nullptr;
}


size_t Processor::getAgentCount() const
{
  return _agents.size();
}


IRAgentContainer Processor::getAgents() const
{
  return _agents;
}


void Processor::clearConvolvers()
{
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    _reverse = false;
    _predelayMs = 0.0;
    _stretch = 1.0;
    _irBegin = 0.0;
    _irEnd = 1.0;
    _attackLength = 0.0;
    _attackShape = 0.0;
    _decayShape = 0.0;
  }

  setParameterNotifyingHost(Parameters::EqLowCutFreq, Parameters::EqLowCutFreq.getDefaultValue());
  setParameterNotifyingHost(Parameters::EqLowShelfFreq, Parameters::EqLowShelfFreq.getDefaultValue());
  setParameterNotifyingHost(Parameters::EqLowShelfDecibels, Parameters::EqLowShelfDecibels.getDefaultValue());
  setParameterNotifyingHost(Parameters::EqHighCutFreq, Parameters::EqHighCutFreq.getDefaultValue());
  setParameterNotifyingHost(Parameters::EqHighShelfFreq, Parameters::EqHighShelfFreq.getDefaultValue());
  setParameterNotifyingHost(Parameters::EqHighShelfDecibels, Parameters::EqHighShelfDecibels.getDefaultValue());
  setParameterNotifyingHost(Parameters::StereoWidth, Parameters::StereoWidth.getDefaultValue());

  for (size_t i=0; i<_agents.size(); ++i)
  {
    _agents[i]->clear();
  }

  notifyAboutChange();
  updateConvolvers();
}


void Processor::setStretch(double stretch)
{
  bool changed = false;
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    if (::fabs(_stretch-stretch) > 0.000001)
    {
      _stretch = stretch;
      changed = true;
    }
  }
  if (changed)
  {
    notifyAboutChange();
    updateConvolvers();
  }
}


double Processor::getStretch() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _stretch;
}


void Processor::setReverse(bool reverse)
{
  bool changed = false;
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    if (_reverse != reverse)
    {
      _reverse = reverse;
      changed = true;
    }
  }
  if (changed)
  {
    notifyAboutChange();
    updateConvolvers();
  }
}


bool Processor::getReverse() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _reverse;
}


void Processor::setPredelayMs(double predelayMs)
{
  bool changed = false;
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    if (_predelayMs != predelayMs)
    {
      _predelayMs = predelayMs;
      changed = true;
    }
  }
  if (changed)
  {
    notifyAboutChange();
    updateConvolvers();
  }
}


double Processor::getPredelayMs() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _predelayMs;
}


void Processor::setAttackLength(double length)
{
  bool changed = false;
  length = std::max(0.0, std::min(1.0, length));
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    if (_attackLength != length)
    {
      _attackLength = length;
      changed = true;
    }
  }
  if (changed)
  {
    notifyAboutChange();
    updateConvolvers();
  }
}


double Processor::getAttackLength() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _attackLength;
}


void Processor::setAttackShape(double shape)
{
  bool changed = false;
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    if (_attackShape != shape)
    {
      _attackShape = shape;
      changed = true;
    }
  }
  if (changed)
  {
    notifyAboutChange();
    updateConvolvers();
  }
}


double Processor::getAttackShape() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _attackShape;
}


void Processor::setDecayShape(double shape)
{
  bool changed = false;
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    if (_decayShape != shape)
    {
      _decayShape = shape;
      changed = true;
    }
  }
  if (changed)
  {
    notifyAboutChange();
    updateConvolvers();
  }
}


double Processor::getDecayShape() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _decayShape;
}


void Processor::setIRBegin(double irBegin)
{
  bool changed = false;  
  bool irChanged = false;
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    double irBeginClamped = std::min(_irEnd, std::max(0.0, irBegin));
    if (::fabs(irBeginClamped-irBegin) > 0.0001)
    {
      changed = true;
    }
    if (::fabs(_irBegin-irBeginClamped) > 0.0001)
    {
      _irBegin = irBeginClamped;
      changed = true;
      irChanged = true;
    }
  }
  if (changed)
  {
    notifyAboutChange();
  }
  if (irChanged)
  {
    updateConvolvers();
  }
}


double Processor::getIRBegin() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _irBegin;
}


void Processor::setIREnd(double irEnd)
{
  bool changed = false;
  bool irChanged = false;
  {
    juce::ScopedLock convolverLock(_convolverMutex);
    double irEndClamped = std::min(1.0, std::max(_irBegin, irEnd));
    if (::fabs(irEndClamped-irEnd) > 0.0001)
    {
      changed = true;
    }
    if (::fabs(_irEnd-irEndClamped) > 0.0001)
    {
      _irEnd = irEndClamped;
      changed = true;
      irChanged = true;
    }    
  }
  if (changed)
  {
    notifyAboutChange();    
  }
  if (irChanged)
  {
    updateConvolvers();
  }
}


double Processor::getIREnd() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _irEnd;
}


size_t Processor::getConvolverHeadBlockSize() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _convolverHeadBlockSize;
}


size_t Processor::getConvolverTailBlockSize() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  return _convolverTailBlockSize;
}


size_t Processor::getIRSampleCount() const
{
  size_t maxSampleCount = 0;
  for (auto it=_agents.begin(); it!=_agents.end(); ++it)
  {
    FloatBuffer::Ptr ir = (*it)->getImpulseResponse();
    if (ir)
    {
      maxSampleCount = std::max(maxSampleCount, ir->getSize());
    }
  }
  return maxSampleCount;
}


double Processor::getIRDuration() const
{
  const double sampleRate = getSampleRate();
  return (sampleRate > 0.0001) ? (static_cast<double>(getIRSampleCount()) / sampleRate) : 0.0;
}


double Processor::getMaxFileDuration() const
{
  juce::ScopedLock convolverLock(_convolverMutex);
  double maxDuration = 0.0;
  for (auto it=_agents.begin(); it!=_agents.end(); ++it)
  {
    const size_t sampleCount = (*it)->getFileSampleCount();
    const double sampleRate = (*it)->getFileSampleRate();
    const double duration = static_cast<double>(sampleCount) / sampleRate;
    if (duration > maxDuration)
    {
      maxDuration = duration;
    }
  }
  return maxDuration;
}


bool Processor::irAvailable() const
{
  for (auto it=_agents.begin(); it!=_agents.end(); ++it)
  {
    if ((*it)->getFile().exists())
    {
      return true;
    }
  }
  return false;
}


void Processor::updateConvolvers()
{
  juce::ScopedLock irCalculationlock(_irCalculationMutex);
  if (_irCalculation)
  {
    _irCalculation->stopThread(-1);
    _irCalculation = nullptr;
  }
  _irCalculation = new IRCalculation(*this);
}


float Processor::getBeatsPerMinute() const
{
  return _beatsPerMinute.get();
}
