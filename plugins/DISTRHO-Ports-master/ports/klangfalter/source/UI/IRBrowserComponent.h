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

#ifndef _IRBROWSERCOMPONENT_H
#define _IRBROWSERCOMPONENT_H


#include "../JuceHeader.h"

#include "../Processor.h"
#include "../Settings.h"


class IRBrowserComponent : public juce::Component,
                           public juce::FileBrowserListener,
                           public juce::ChangeListener
{
public:
  IRBrowserComponent();
  virtual ~IRBrowserComponent();
  
  virtual void init(Processor* processor);
  virtual void updateLayout();
  
  virtual void paint(juce::Graphics& g);
  virtual void resized();
  
  virtual void selectionChanged();
  virtual void fileClicked(const juce::File &file, const juce::MouseEvent &e);
 	virtual void fileDoubleClicked(const juce::File &file);
 	virtual void browserRootChanged(const juce::File &newRoot);

  virtual void changeListenerCallback(juce::ChangeBroadcaster* source);
  
private:
  bool readAudioFileInfo(const juce::File& file, size_t& channelCount, size_t& sampleCount, double& sampleRate) const;

  typedef std::vector<std::pair<juce::File, size_t> > TrueStereoPairs;
  TrueStereoPairs findTrueStereoPairs(const juce::File& file, size_t sampleCount, double sampleRate) const;
  juce::File checkMatchingTrueStereoFile(const juce::String& fileNameBody,
                                         const juce::String& fileNameExt,
                                         const juce::File& directory,
                                         const juce::String& pattern,
                                         const juce::String& replacement,
                                         const size_t sampleCount,
                                         const double sampleRate) const;

  juce::ScopedPointer<juce::TimeSliceThread> _timeSliceThread;
  juce::ScopedPointer<juce::FileFilter> _fileFilter;
  juce::ScopedPointer<juce::DirectoryContentsList> _directoryContent;
  juce::ScopedPointer<juce::FileTreeComponent> _fileTreeComponent;
  juce::ScopedPointer<juce::Label> _infoLabel;
  Processor* _processor;
  
  IRBrowserComponent(const IRBrowserComponent&);
  IRBrowserComponent& operator=(const IRBrowserComponent&);
};

#endif // Header guard
