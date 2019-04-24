/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  8 Mar 2013 10:37:54am

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Jucer version: 1.12

  ------------------------------------------------------------------------------

  The Jucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-6 by Raw Material Software ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
#include "../IRAgent.h"
#include "../Settings.h"
//[/Headers]

#include "IRComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
IRComponent::IRComponent ()
    : Component (L"IRComponent"),
      _waveformComponent (0),
      _loadButton (0),
      _clearButton (0),
      _channelComboBox (0),
      _channelHeaderLabel (0)
{
    addAndMakeVisible (_waveformComponent = new WaveformComponent());
    _waveformComponent->setName (L"WaveformComponent");

    addAndMakeVisible (_loadButton = new TextButton (L"LoadButton"));
    _loadButton->setTooltip (L"Click To Change Impulse Response");
    _loadButton->setButtonText (L"No Impulse Response");
    _loadButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _loadButton->addListener (this);
    _loadButton->setColour (TextButton::buttonColourId, Colour (0xbbbbff));
    _loadButton->setColour (TextButton::buttonOnColourId, Colour (0x4444ff));
    _loadButton->setColour (TextButton::textColourOnId, Colour (0xff202020));
    _loadButton->setColour (TextButton::textColourOffId, Colour (0xff202020));

    addAndMakeVisible (_clearButton = new TextButton (L"ClearButton"));
    _clearButton->setTooltip (L"Clear Impulse Response");
    _clearButton->setButtonText (L"X");
    _clearButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _clearButton->addListener (this);

    addAndMakeVisible (_channelComboBox = new ComboBox (L"ChannelComboBox"));
    _channelComboBox->setTooltip (L"Select Channel Of Currently Loaded Audio File");
    _channelComboBox->setEditableText (false);
    _channelComboBox->setJustificationType (Justification::centred);
    _channelComboBox->setTextWhenNothingSelected (String());
    _channelComboBox->setTextWhenNoChoicesAvailable (L"(no choices)");
    _channelComboBox->addListener (this);

    addAndMakeVisible (_channelHeaderLabel = new Label (String(),
                                                        L"Channel:"));
    _channelHeaderLabel->setFont (Font (15.0000f, Font::plain));
    _channelHeaderLabel->setJustificationType (Justification::centredLeft);
    _channelHeaderLabel->setEditable (false, false, false);
    _channelHeaderLabel->setColour (Label::textColourId, Colour (0xff202020));
    _channelHeaderLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _channelHeaderLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));


    //[UserPreSize]
    //[/UserPreSize]

    setSize (540, 172);


    //[Constructor] You can add your own custom stuff here..
    _irAgent = nullptr;
    //[/Constructor]
}

IRComponent::~IRComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    IRComponent::init(nullptr);
    //[/Destructor_pre]

    deleteAndZero (_waveformComponent);
    deleteAndZero (_loadButton);
    deleteAndZero (_clearButton);
    deleteAndZero (_channelComboBox);
    deleteAndZero (_channelHeaderLabel);


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void IRComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xffb0b0b6));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void IRComponent::resized()
{
    _waveformComponent->setBounds (4, 4, 532, 140);
    _loadButton->setBounds (4, 148, 396, 24);
    _clearButton->setBounds (516, 148, 20, 20);
    _channelComboBox->setBounds (468, 148, 40, 20);
    _channelHeaderLabel->setBounds (404, 152, 64, 15);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void IRComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == _loadButton)
    {
        //[UserButtonCode__loadButton] -- add your button handler code here..
        AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        FileChooser fileChooser("Choose a file to open...",
                                _irAgent->getProcessor().getSettings().getImpulseResponseDirectory(),
                                formatManager.getWildcardForAllFormats(),
                                true);
        if (fileChooser.browseForFileToOpen() && fileChooser.getResults().size() == 1)
        {
          const File file = fileChooser.getResults().getReference(0);
          _irAgent->setFile(file, 0);
        }
        //[/UserButtonCode__loadButton]
    }
    else if (buttonThatWasClicked == _clearButton)
    {
        //[UserButtonCode__clearButton] -- add your button handler code here..
        if (_irAgent)
        {
          _irAgent->clear();
        }
        //[/UserButtonCode__clearButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}

void IRComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == _channelComboBox)
    {
        //[UserComboBoxCode__channelComboBox] -- add your combo box handling code here..
        if (_irAgent)
        {
          const int selectedId = comboBoxThatHasChanged->getSelectedId();
          _irAgent->setFile(_irAgent->getFile(), static_cast<size_t>(std::max(0, selectedId-1)));
        }
        //[/UserComboBoxCode__channelComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void IRComponent::init(IRAgent* irAgent)
{
  if (_irAgent)
  {
    _irAgent->removeNotificationListener(this);
    _irAgent->getProcessor().removeNotificationListener(this);
    _irAgent = nullptr;
  }

  if (irAgent)
  {
    _irAgent = irAgent;
    _irAgent->addNotificationListener(this);
    _irAgent->getProcessor().addNotificationListener(this);
  }
  irChanged();
}


void IRComponent::irChanged()
{
  String fileLabelText("No File Loaded");

  _channelComboBox->clear();

  juce::String toolTip("No Impulse Response");

  double sampleRate = 0.0;
  size_t samplesPerPx = 0;
  
  if (_irAgent)
  {
    const File file = _irAgent->getFile();
    if (file.exists())
    {
      const Processor& processor = _irAgent->getProcessor();
      const unsigned fileSampleCount = static_cast<unsigned>(_irAgent->getFileSampleCount());
      const double fileSampleRate = _irAgent->getFileSampleRate();
      const double fileSeconds = static_cast<double>(fileSampleCount) / fileSampleRate;
      const unsigned fileChannelCount = static_cast<unsigned>(_irAgent->getFileChannelCount());
      fileLabelText = file.getFileName() + String(", ") + String(fileChannelCount) + String(" Channels, ") + String(fileSeconds, 2) + String(" s");

      for (size_t i=0; i<fileChannelCount; ++i)
      {
        _channelComboBox->addItem(String(static_cast<int>(i+1)), i+1);
      }
      _channelComboBox->setSelectedId(static_cast<int>(_irAgent->getFileChannel()+1));

      sampleRate = processor.getSampleRate();
      samplesPerPx = static_cast<size_t>(1.6 * (processor.getMaxFileDuration()+1.0) * sampleRate) / _waveformComponent->getWidth();
    }
  }
  _waveformComponent->init(_irAgent, sampleRate, samplesPerPx);
  _loadButton->setButtonText(fileLabelText);
}


void IRComponent::changeNotification()
{
  irChanged();
}


//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="IRComponent" componentName="IRComponent"
                 parentClasses="public Component, public ChangeNotifier::Listener"
                 constructorParams="" variableInitialisers="" snapPixels="4" snapActive="1"
                 snapShown="1" overlayOpacity="0.330000013" fixedSize="1" initialWidth="540"
                 initialHeight="172">
  <BACKGROUND backgroundColour="ffb0b0b6"/>
  <GENERICCOMPONENT name="WaveformComponent" id="c9f33b0ee0917f49" memberName="_waveformComponent"
                    virtualName="" explicitFocusOrder="0" pos="4 4 532 140" class="WaveformComponent"
                    params=""/>
  <TEXTBUTTON name="LoadButton" id="5798b8525a699c54" memberName="_loadButton"
              virtualName="" explicitFocusOrder="0" pos="4 148 396 24" tooltip="Click To Change Impulse Response"
              bgColOff="bbbbff" bgColOn="4444ff" textCol="ff202020" textColOn="ff202020"
              buttonText="No Impulse Response" connectedEdges="3" needsCallback="1"
              radioGroupId="0"/>
  <TEXTBUTTON name="ClearButton" id="6bd842f223117695" memberName="_clearButton"
              virtualName="" explicitFocusOrder="0" pos="516 148 20 20" tooltip="Clear Impulse Response"
              buttonText="X" connectedEdges="3" needsCallback="1" radioGroupId="0"/>
  <COMBOBOX name="ChannelComboBox" id="c1bafd26d5583017" memberName="_channelComboBox"
            virtualName="" explicitFocusOrder="0" pos="468 148 40 20" tooltip="Select Channel Of Currently Loaded Audio File"
            editable="0" layout="36" items="" textWhenNonSelected="" textWhenNoItems="(no choices)"/>
  <LABEL name="" id="7dd1e4a23ce6582b" memberName="_channelHeaderLabel"
         virtualName="" explicitFocusOrder="0" pos="404 152 64 15" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Channel:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif
