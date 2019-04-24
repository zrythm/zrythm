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

#include "IRBrowserComponent.h"

#include "../IRAgent.h"
#include "../Settings.h"


IRBrowserComponent::IRBrowserComponent() :
  juce::Component(),
  _timeSliceThread(),
  _fileFilter(),
  _directoryContent(),
  _fileTreeComponent(),
  _infoLabel(),
  _processor(nullptr)
{
}


IRBrowserComponent::~IRBrowserComponent()
{
  if (_processor)
  {
    _processor->getSettings().removeChangeListener(this);
  }

  _processor = nullptr;
  _fileTreeComponent = nullptr;
  _directoryContent = nullptr;
  _fileFilter = nullptr;
  _timeSliceThread = nullptr;
}


void IRBrowserComponent::init(Processor* processor)
{
  _processor = processor;
  Settings* settings = _processor ? &(_processor->getSettings()) : nullptr;
  if (settings)
  {
    settings->addChangeListener(this);
  }

  if (!_timeSliceThread)
  {
    _timeSliceThread = new juce::TimeSliceThread("IRBrowserThread");
    _timeSliceThread->startThread();
  }
  
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  _fileFilter = new juce::WildcardFileFilter(formatManager.getWildcardForAllFormats(),
                                                                      "*",
                                                                      "Audio Files");
  
  _directoryContent = new juce::DirectoryContentsList(_fileFilter, *_timeSliceThread);
  _directoryContent->setDirectory(settings ? settings->getImpulseResponseDirectory() : juce::File(), true, true);
  
  _fileTreeComponent = new juce::FileTreeComponent(*_directoryContent);
  _fileTreeComponent->addListener(this);
  addAndMakeVisible(_fileTreeComponent);
  
  _infoLabel = new juce::Label();
  addAndMakeVisible(_infoLabel);

  updateLayout();
}


void IRBrowserComponent::updateLayout()
{
  if (_fileTreeComponent && _infoLabel)
  {
    const int width = getWidth();
    const int height = getHeight();
    
    const int treeWidth = std::min(static_cast<int>(0.75 * static_cast<double>(width)), width - 280) - 2;
    const int treeHeight = height - 2;
    const int infoMargin = 8;
    const int infoX = treeWidth + infoMargin;
    const int infoWidth = width - (infoX + infoMargin);
    const int infoHeight = height - (2 * infoMargin); 
    
    _fileTreeComponent->setBounds(1, 1, treeWidth, treeHeight);
    _infoLabel->setBounds(infoX, infoMargin, infoWidth, infoHeight);
  }
}


void IRBrowserComponent::paint(juce::Graphics& g)
{
  if (_fileTreeComponent && _infoLabel)
  {
    const int width = getWidth();
    const int height = getHeight();
    
    g.setColour(juce::Colour(0xE5, 0xE5, 0xF0));
    g.fillRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    
    g.setColour(juce::Colours::grey);
    g.drawRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    g.drawVerticalLine(_fileTreeComponent->getX()+_fileTreeComponent->getWidth(), 0.0f, static_cast<float>(height-1));
  }
}


void IRBrowserComponent::resized()
{
  updateLayout();
}


void IRBrowserComponent::selectionChanged()
{
  if (_infoLabel)
  {
    juce::String infoText;
    
    const juce::File file = _fileTreeComponent ? _fileTreeComponent->getSelectedFile() : juce::File();

    if (!file.isDirectory() && _processor)
    {
      size_t channelCount = 0;
      size_t sampleCount = 0;
      double sampleRate = 0.0;
      if (readAudioFileInfo(file, channelCount, sampleCount, sampleRate))
      {
        infoText += juce::String("Name: ") + file.getFileName();
        infoText += juce::String("\n");
        infoText += juce::String("\nSamples: ") + juce::String(static_cast<int>(sampleCount));
        if (sampleRate > 0.00001)
        {
          const double seconds = static_cast<double>(sampleCount) / sampleRate;
          infoText += juce::String("\nDuration: ") + juce::String(seconds, 2) + juce::String("s");
        }
        infoText += juce::String("\nChannels: ") + juce::String(static_cast<int>(channelCount));
        infoText += juce::String("\nSample Rate: ") + juce::String(static_cast<int>(sampleRate)) + juce::String("Hz");

        if (_processor->getTotalNumInputChannels() >= 2 && _processor->getTotalNumOutputChannels() >= 2)
        {
          const TrueStereoPairs trueStereoPairs = findTrueStereoPairs(file, sampleCount, sampleRate);        
          for (size_t i=0; i<trueStereoPairs.size(); ++i)
          {
            if (trueStereoPairs[i].first != file && trueStereoPairs[i].first.exists())
            {
              infoText += juce::String("\n");
              infoText += juce::String("\nFile Pair For True-Stereo:");
              infoText += juce::String("\n - ") + file.getFileName();
              infoText += juce::String("\n - ") + trueStereoPairs[i].first.getFileName();
              break;
            }
          }
        }
      }
      else
      {
        infoText += juce::String("\n\nError!\n\nNo information available.");
      }
    }
    
    _infoLabel->setJustificationType(juce::Justification(juce::Justification::topLeft));
    _infoLabel->setText(infoText, juce::sendNotification);
  }
}


void IRBrowserComponent::fileClicked(const File&, const MouseEvent&)
{
}


void IRBrowserComponent::fileDoubleClicked(const File &file)
{
  if (!_processor || file.isDirectory())
  {
    return;
  }
  
  size_t channelCount = 0;
  size_t sampleCount = 0;
  double sampleRate = 0.0;
  if (!readAudioFileInfo(file, channelCount, sampleCount, sampleRate))
  {
    return;
  }
  
  IRAgent* agent00 = _processor->getAgent(0, 0);
  IRAgent* agent01 = _processor->getAgent(0, 1);
  IRAgent* agent10 = _processor->getAgent(1, 0);
  IRAgent* agent11 = _processor->getAgent(1, 1);
  
  const int inputChannels = _processor->getTotalNumInputChannels();
  const int outputChannels = _processor->getTotalNumOutputChannels();
  
  if (inputChannels == 1 && outputChannels == 1)
  {
    if (channelCount >= 1)
    {
      _processor->clearConvolvers();
      agent00->setFile(file, 0);
    }
  }
  else if (inputChannels == 1 && outputChannels == 2)
  {
    if (channelCount == 1)
    {
      _processor->clearConvolvers();
      agent00->setFile(file, 0);
      agent01->setFile(file, 0);
    }
    else if (channelCount >= 2)
    {
      _processor->clearConvolvers();
      agent00->setFile(file, 0);
      agent01->setFile(file, 1);
    }
  }
  else if (inputChannels == 2 && outputChannels == 2)
  {
    if (channelCount == 1)
    {
      _processor->clearConvolvers();
      agent00->setFile(file, 0);
      agent11->setFile(file, 0);
    }
    else if (channelCount == 2)
    {
      TrueStereoPairs trueStereoPairs = findTrueStereoPairs(file, sampleCount, sampleRate);
      if (trueStereoPairs.size() == 4)
      {
        _processor->clearConvolvers();
        agent00->setFile(trueStereoPairs[0].first, trueStereoPairs[0].second);
        agent01->setFile(trueStereoPairs[1].first, trueStereoPairs[1].second);
        agent10->setFile(trueStereoPairs[2].first, trueStereoPairs[2].second);
        agent11->setFile(trueStereoPairs[3].first, trueStereoPairs[3].second);
      }
      else
      {
        _processor->clearConvolvers();
        agent00->setFile(file, 0);
        agent11->setFile(file, 1);
      }
    }
    else if (channelCount >= 4)
    {
      _processor->clearConvolvers();
      agent00->setFile(file, 0);
      agent01->setFile(file, 1);
      agent10->setFile(file, 2);
      agent11->setFile(file, 3);
    }
  }
}


void IRBrowserComponent::browserRootChanged(const File&)
{
}


void IRBrowserComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
  if (_directoryContent && _processor)
  {
    _directoryContent->setDirectory(_processor->getSettings().getImpulseResponseDirectory(), true, true);
  }
}


bool IRBrowserComponent::readAudioFileInfo(const juce::File& file, size_t& channelCount, size_t& sampleCount, double& sampleRate) const
{
  juce::AudioFormatManager formatManager;
  formatManager.registerBasicFormats();
  juce::ScopedPointer<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
  if (reader)
  {
    channelCount = static_cast<size_t>(reader->numChannels);
    sampleCount = static_cast<size_t>(reader->lengthInSamples);
    sampleRate = reader->sampleRate;
    return true;
  }
  channelCount = 0;
  sampleCount = 0;
  sampleRate = 0.0;
  return false;
}


IRBrowserComponent::TrueStereoPairs IRBrowserComponent::findTrueStereoPairs(const juce::File& file, size_t sampleCount, double sampleRate) const
{
  if (! file.exists() || file.isDirectory())
  {
    return TrueStereoPairs();
  }

  const juce::File directory = file.getParentDirectory();
  if (! file.exists() || file.isDirectory())
  {
    return TrueStereoPairs();
  }

  const juce::String fileNameBody = file.getFileNameWithoutExtension();
  const juce::String fileNameExt = file.getFileExtension();

  // Left => Right
  static std::vector<std::pair<juce::String, juce::String> > pairsLeft;
  if (pairsLeft.empty())
  {
    pairsLeft.push_back(std::make_pair(juce::String("L"),    juce::String("R")));
    pairsLeft.push_back(std::make_pair(juce::String("l"),    juce::String("r")));
    pairsLeft.push_back(std::make_pair(juce::String("Left"), juce::String("Right")));
    pairsLeft.push_back(std::make_pair(juce::String("left"), juce::String("right")));
    pairsLeft.push_back(std::make_pair(juce::String("LEFT"), juce::String("RIGHT")));
  }
  for (size_t i=0; i<pairsLeft.size(); ++i)
  {
    const juce::File matchingFile = checkMatchingTrueStereoFile(fileNameBody,
                                                                fileNameExt,
                                                                directory,
                                                                pairsLeft[i].first,
                                                                pairsLeft[i].second,
                                                                sampleCount,
                                                                sampleRate);
    if (matchingFile.exists())
    {
      TrueStereoPairs trueStereoPairs;
      trueStereoPairs.push_back(std::make_pair(file, 0));
      trueStereoPairs.push_back(std::make_pair(file, 1));
      trueStereoPairs.push_back(std::make_pair(matchingFile, 0));
      trueStereoPairs.push_back(std::make_pair(matchingFile, 1));
      return trueStereoPairs;
    }
  }

  static std::vector<std::pair<juce::String, juce::String> > pairsRight;
  if (pairsRight.empty())
  {
    pairsRight.push_back(std::make_pair(juce::String("R"),     juce::String("L")));
    pairsRight.push_back(std::make_pair(juce::String("r"),     juce::String("l")));
    pairsRight.push_back(std::make_pair(juce::String("Right"), juce::String("Left")));
    pairsRight.push_back(std::make_pair(juce::String("right"), juce::String("left")));
    pairsRight.push_back(std::make_pair(juce::String("RIGHT"), juce::String("LEFT")));
  }
  for (size_t i=0; i<pairsRight.size(); ++i)
  {
    const juce::File matchingFile = checkMatchingTrueStereoFile(fileNameBody,
                                                                fileNameExt,
                                                                directory,
                                                                pairsRight[i].first,
                                                                pairsRight[i].second,
                                                                sampleCount,
                                                                sampleRate);
    if (matchingFile.exists())
    {
      TrueStereoPairs trueStereoPairs;
      trueStereoPairs.push_back(std::make_pair(matchingFile, 0));
      trueStereoPairs.push_back(std::make_pair(matchingFile, 1));
      trueStereoPairs.push_back(std::make_pair(file, 0));
      trueStereoPairs.push_back(std::make_pair(file, 1));
      return trueStereoPairs;
    }
  }

  return TrueStereoPairs();
}



juce::File IRBrowserComponent::checkMatchingTrueStereoFile(const juce::String& fileNameBody,
                                                           const juce::String& fileNameExt,
                                                           const juce::File& directory,
                                                           const juce::String& pattern,
                                                           const juce::String& replacement,
                                                           const size_t sampleCount,
                                                           const double sampleRate) const
{
  std::vector<juce::String> candidateNames;
  if (fileNameBody.startsWith(pattern))
  {
    candidateNames.push_back(replacement + fileNameBody.substring(pattern.length(), fileNameBody.length()) + fileNameExt);
  }
  if (fileNameBody.endsWith(pattern))
  {
    candidateNames.push_back(fileNameBody.substring(0, fileNameBody.length()-pattern.length()) + replacement + fileNameExt);
  }

  for (size_t i=0; i<candidateNames.size(); ++i)
  {
    const juce::String& candidateName = candidateNames[i];
    if (directory.getNumberOfChildFiles(juce::File::findFiles|juce::File::ignoreHiddenFiles, candidateName) == 1)
    {
      const juce::File candidateFile = directory.getChildFile(candidateName);
      size_t candidateChannelCount = 0;
      size_t candidateSampleCount = 0;
      double candidateSampleRate = 0.0;
      const bool fileInfoSuccess = readAudioFileInfo(candidateFile, candidateChannelCount, candidateSampleCount, candidateSampleRate);
      if (fileInfoSuccess &&
          candidateChannelCount == 2 &&
          candidateSampleCount == sampleCount &&
          ::fabs(candidateSampleRate - sampleRate) < 0.000001)
      {
        return candidateFile;
      }
    }
  }

  return juce::File();
}
