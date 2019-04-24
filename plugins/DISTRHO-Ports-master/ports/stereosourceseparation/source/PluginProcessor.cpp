/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define FFT_SIZE 4096

//==============================================================================
StereoSourceSeparationAudioProcessor::StereoSourceSeparationAudioProcessor()
    :NUM_CHANNELS(2),
    BLOCK_SIZE(FFT_SIZE),
    HOP_SIZE(FFT_SIZE/4),
    BETA(100),
    inputBuffer_(2,FFT_SIZE),
    outputBuffer_(2,FFT_SIZE*2),
    processBuffer_(2, FFT_SIZE)
{
    inputBufferLength_ = FFT_SIZE;
    outputBufferLength_ = FFT_SIZE*2;
    separator_ = 0;
    
    status_ = ADRess::kBypass;
    direction_ = BETA/2;
    width_ = BETA/4;
    filterType_ = ADRess::kAllPass;
    cutOffFrequency_ = 0.0;
    
}

StereoSourceSeparationAudioProcessor::~StereoSourceSeparationAudioProcessor()
{
}

//==============================================================================
const String StereoSourceSeparationAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

int StereoSourceSeparationAudioProcessor::getNumParameters()
{
    return kNumParameters;
}

float StereoSourceSeparationAudioProcessor::getParameter (int index)
{
    switch (index) {
        case kStatus:
            return (float)status_;
            
        case kDirection:
            return (float)direction_;
            
        case kWidth:
            return (float)width_;
            
        case kFilterType:
            return (float)filterType_;
            
        case kCutOffFrequency:
            return cutOffFrequency_;
            
        default:
            return 0.0f;
    }
}

void StereoSourceSeparationAudioProcessor::setParameter (int index, float newValue)
{
    switch (index) {
        case kStatus:
            status_ = static_cast<ADRess::Status_t>(newValue);
            if (separator_)
                separator_->setStatus( status_ );
            break;
            
        case kDirection:
            direction_ = static_cast<int>(newValue);
            if (separator_)
                separator_->setDirection( direction_ );
            break;
            
        case kWidth:
            width_ = static_cast<int>(newValue);
            if (separator_)
                separator_->setWidth( width_ );
            break;
            
        case kFilterType:
            filterType_ = static_cast<ADRess::FilterType_t>(newValue);
            if (separator_)
                separator_->setFilterType(filterType_);
                
            break;
            
        case kCutOffFrequency:
            cutOffFrequency_ = newValue;
            if (separator_)
                separator_->setCutOffFrequency(cutOffFrequency_);
            
            break;
            
        default:
            break;
    }
}

const String StereoSourceSeparationAudioProcessor::getParameterName (int index)
{
    switch (index) {
        case kStatus:
            return "Status";
            
        case kDirection:
            return "Direction";
            
        case kWidth:
            return "Width";
            
        case kFilterType:
            return "FilterType";
            
        case kCutOffFrequency:
            return "CutOffFrequency";
            
        default:
            return String();
    }
}

const String StereoSourceSeparationAudioProcessor::getParameterText (int index)
{
    return String(getParameter(index));
}

const String StereoSourceSeparationAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

const String StereoSourceSeparationAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String (channelIndex + 1);
}

bool StereoSourceSeparationAudioProcessor::isInputChannelStereoPair (int index) const
{
    return true;
}

bool StereoSourceSeparationAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return true;
}

bool StereoSourceSeparationAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool StereoSourceSeparationAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool StereoSourceSeparationAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double StereoSourceSeparationAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int StereoSourceSeparationAudioProcessor::getNumPrograms()
{
    return 0;
}

int StereoSourceSeparationAudioProcessor::getCurrentProgram()
{
    return 0;
}

void StereoSourceSeparationAudioProcessor::setCurrentProgram (int index)
{
}

const String StereoSourceSeparationAudioProcessor::getProgramName (int index)
{
    return String();
}

void StereoSourceSeparationAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void StereoSourceSeparationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    separator_ = new ADRess(sampleRate, BLOCK_SIZE, BETA);
    
    inputBuffer_.clear();
    outputBuffer_.clear();
    processBuffer_.clear();
    inputBufferWritePosition_ = outputBufferWritePosition_ = 0;
    outputBufferReadPosition_ = (outputBufferLength_ - HOP_SIZE - 1) % outputBufferLength_;
    samplesSinceLastFFT_ = 0;
    
    separator_->setStatus( status_ );
    separator_->setDirection( direction_ );
    separator_->setWidth( width_ );
    separator_->setFilterType(filterType_);
    separator_->setCutOffFrequency(cutOffFrequency_);
}

void StereoSourceSeparationAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    delete separator_;
}

void StereoSourceSeparationAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    // get pointers for buffer access
    float* stereoData[NUM_CHANNELS];
    stereoData[0] = buffer.getWritePointer(0);
    stereoData[1] = buffer.getWritePointer(1);
    float* inputBufferData[NUM_CHANNELS];
    inputBufferData[0] = inputBuffer_.getWritePointer(0);
    inputBufferData[1] = inputBuffer_.getWritePointer(1);
    float* outputBufferData[NUM_CHANNELS];
    outputBufferData[0] = outputBuffer_.getWritePointer(0);
    outputBufferData[1] = outputBuffer_.getWritePointer(1);
    float* processBufferData[NUM_CHANNELS];
    processBufferData[0] = processBuffer_.getWritePointer(0);
    processBufferData[1] = processBuffer_.getWritePointer(1);
    
    
    for (int i = 0; i<buffer.getNumSamples(); i++) {
        
        // store input sample data in input buffer
        inputBufferData[0][inputBufferWritePosition_] = stereoData[0][i];
        inputBufferData[1][inputBufferWritePosition_] = stereoData[1][i];
        if (++inputBufferWritePosition_ >= inputBufferLength_)
            inputBufferWritePosition_ = 0;
        
        // output sample from output buffer
        stereoData[0][i] = outputBufferData[0][outputBufferReadPosition_];
        stereoData[1][i] = outputBufferData[1][outputBufferReadPosition_];
        
        // clear output buffer sample in preparation for next overlap and add
        outputBufferData[0][outputBufferReadPosition_] = 0.0;
        outputBufferData[1][outputBufferReadPosition_] = 0.0;
        if (++outputBufferReadPosition_ >= outputBufferLength_)
            outputBufferReadPosition_ = 0;
        
        // if samples since last fft exceeds hopsize, do fft
        if (++samplesSinceLastFFT_ >= HOP_SIZE) {
            samplesSinceLastFFT_ = 0;
            
            // find start positino in input buffer, then load the process buffer leftData_ and rightData_
            int inputBufferIndex = (inputBufferWritePosition_ - BLOCK_SIZE + inputBufferLength_) % inputBufferLength_;
            for (int procBufferIndex = 0; procBufferIndex < BLOCK_SIZE; procBufferIndex++) {
                processBufferData[0][procBufferIndex] = inputBufferData[0][inputBufferIndex];
                processBufferData[1][procBufferIndex] = inputBufferData[1][inputBufferIndex];
                if ( ++inputBufferIndex >= inputBufferLength_)
                    inputBufferIndex = 0;
            }
            
            // performs source separation here
            separator_->process(processBufferData[0], processBufferData[1]);
            
            // overlap and add in output buffer
            int outputBufferIndex = outputBufferWritePosition_;
            for (int procBufferIndex = 0; procBufferIndex < BLOCK_SIZE; procBufferIndex++) {
                outputBufferData[0][outputBufferIndex] += processBufferData[0][procBufferIndex];
                outputBufferData[1][outputBufferIndex] += processBufferData[1][procBufferIndex];
                if (++outputBufferIndex >= outputBufferLength_)
                    outputBufferIndex = 0;
            }
            
            // advance write position by hop size
            outputBufferWritePosition_ = (outputBufferWritePosition_ + HOP_SIZE) % outputBufferLength_;
        }
        
    }
    

    // In case we have more outputs than inputs, we'll clear any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
}

//==============================================================================
bool StereoSourceSeparationAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* StereoSourceSeparationAudioProcessor::createEditor()
{
    return new StereoSourceSeparationAudioProcessorEditor (this);
}

//==============================================================================
void StereoSourceSeparationAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void StereoSourceSeparationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoSourceSeparationAudioProcessor();
}
