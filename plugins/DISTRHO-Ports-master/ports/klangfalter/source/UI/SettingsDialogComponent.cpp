/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  15 May 2013 4:41:55pm

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

#include "../Settings.h"

//[/Headers]

#include "SettingsDialogComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
SettingsDialogComponent::SettingsDialogComponent (Processor& processor)
    : _processor(processor),
      _irDirectoryGroupComponent (0),
      _aboutGroupComponent (0),
      _nameVersionLabel (0),
      _copyrightLabel (0),
      _myLabel (0),
      _licenseHyperlink (0),
      _infoGroupComponent (0),
      _juceVersionPrefixLabel (0),
      _juceVersionLabel (0),
      _numberInputsPrefixLabel (0),
      _numberInputsLabel (0),
      _numberOutputsPrefixLabel (0),
      _numberOutputsLabel (0),
      _sseOptimizationPrefixLabel (0),
      _sseOptimizationLabel (0),
      _headBlockSizePrefixLabel (0),
      _headBlockSizeLabel (0),
      _tailBlockSizePrefixLabel (0),
      _tailBlockSizeLabel (0),
      _selectIRDirectoryButton (0),
      cachedImage_hifilofi_jpg (0)
{
    addAndMakeVisible (_irDirectoryGroupComponent = new GroupComponent (String(),
                                                                        L"Impulse Response Directory"));
    _irDirectoryGroupComponent->setColour (GroupComponent::textColourId, Colour (0xff202020));

    addAndMakeVisible (_aboutGroupComponent = new GroupComponent (String(),
                                                                  L"About"));
    _aboutGroupComponent->setColour (GroupComponent::textColourId, Colour (0xff202020));

    addAndMakeVisible (_nameVersionLabel = new Label (String(),
                                                      L"KlangFalter - Version <Unknown>"));
    _nameVersionLabel->setFont (Font (15.0000f, Font::plain));
    _nameVersionLabel->setJustificationType (Justification::centredLeft);
    _nameVersionLabel->setEditable (false, false, false);
    _nameVersionLabel->setColour (Label::textColourId, Colour (0xff202020));
    _nameVersionLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _nameVersionLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_copyrightLabel = new Label (String(),
                                                    L"Copyright (c) 2013 HiFi-LoFi"));
    _copyrightLabel->setFont (Font (15.0000f, Font::plain));
    _copyrightLabel->setJustificationType (Justification::centredLeft);
    _copyrightLabel->setEditable (false, false, false);
    _copyrightLabel->setColour (Label::textColourId, Colour (0xff202020));
    _copyrightLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _copyrightLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_myLabel = new Label (String(),
                                             L"Modified by falkTX"));
    _myLabel->setFont (Font (15.0000f, Font::plain));
    _myLabel->setJustificationType (Justification::centredLeft);
    _myLabel->setEditable (false, false, false);
    _myLabel->setColour (Label::textColourId, Colour (0xff202020));
    _myLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _myLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_licenseHyperlink = new HyperlinkButton (L"Licensed under GPL3",
                                                                URL (L"http://www.gnu.org/licenses")));
    _licenseHyperlink->setTooltip (L"http://www.gnu.org/licenses");
    _licenseHyperlink->setButtonText (L"Licensed under GPL3");

    addAndMakeVisible (_infoGroupComponent = new GroupComponent (String(),
                                                                 L"Plugin Information"));
    _infoGroupComponent->setColour (GroupComponent::textColourId, Colour (0xff202020));

    addAndMakeVisible (_juceVersionPrefixLabel = new Label (String(),
                                                            L"JUCE Version:"));
    _juceVersionPrefixLabel->setFont (Font (15.0000f, Font::plain));
    _juceVersionPrefixLabel->setJustificationType (Justification::centredLeft);
    _juceVersionPrefixLabel->setEditable (false, false, false);
    _juceVersionPrefixLabel->setColour (Label::textColourId, Colour (0xff202020));
    _juceVersionPrefixLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _juceVersionPrefixLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_juceVersionLabel = new Label (String(),
                                                      L"<Unknown>"));
    _juceVersionLabel->setFont (Font (15.0000f, Font::plain));
    _juceVersionLabel->setJustificationType (Justification::centredLeft);
    _juceVersionLabel->setEditable (false, false, false);
    _juceVersionLabel->setColour (Label::textColourId, Colour (0xff202020));
    _juceVersionLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _juceVersionLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_numberInputsPrefixLabel = new Label (String(),
                                                             L"Input Channels:"));
    _numberInputsPrefixLabel->setFont (Font (15.0000f, Font::plain));
    _numberInputsPrefixLabel->setJustificationType (Justification::centredLeft);
    _numberInputsPrefixLabel->setEditable (false, false, false);
    _numberInputsPrefixLabel->setColour (Label::textColourId, Colour (0xff202020));
    _numberInputsPrefixLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _numberInputsPrefixLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_numberInputsLabel = new Label (String(),
                                                       L"<Unknown>"));
    _numberInputsLabel->setFont (Font (15.0000f, Font::plain));
    _numberInputsLabel->setJustificationType (Justification::centredLeft);
    _numberInputsLabel->setEditable (false, false, false);
    _numberInputsLabel->setColour (Label::textColourId, Colour (0xff202020));
    _numberInputsLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _numberInputsLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_numberOutputsPrefixLabel = new Label (String(),
                                                              L"Output Channels:"));
    _numberOutputsPrefixLabel->setFont (Font (15.0000f, Font::plain));
    _numberOutputsPrefixLabel->setJustificationType (Justification::centredLeft);
    _numberOutputsPrefixLabel->setEditable (false, false, false);
    _numberOutputsPrefixLabel->setColour (Label::textColourId, Colour (0xff202020));
    _numberOutputsPrefixLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _numberOutputsPrefixLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_numberOutputsLabel = new Label (String(),
                                                        L"<Unknown>"));
    _numberOutputsLabel->setFont (Font (15.0000f, Font::plain));
    _numberOutputsLabel->setJustificationType (Justification::centredLeft);
    _numberOutputsLabel->setEditable (false, false, false);
    _numberOutputsLabel->setColour (Label::textColourId, Colour (0xff202020));
    _numberOutputsLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _numberOutputsLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_sseOptimizationPrefixLabel = new Label (String(),
                                                                L"SSE Optimization:"));
    _sseOptimizationPrefixLabel->setFont (Font (15.0000f, Font::plain));
    _sseOptimizationPrefixLabel->setJustificationType (Justification::centredLeft);
    _sseOptimizationPrefixLabel->setEditable (false, false, false);
    _sseOptimizationPrefixLabel->setColour (Label::textColourId, Colour (0xff202020));
    _sseOptimizationPrefixLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _sseOptimizationPrefixLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_sseOptimizationLabel = new Label (String(),
                                                          L"<Unknown>"));
    _sseOptimizationLabel->setFont (Font (15.0000f, Font::plain));
    _sseOptimizationLabel->setJustificationType (Justification::centredLeft);
    _sseOptimizationLabel->setEditable (false, false, false);
    _sseOptimizationLabel->setColour (Label::textColourId, Colour (0xff202020));
    _sseOptimizationLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _sseOptimizationLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_headBlockSizePrefixLabel = new Label (String(),
                                                              L"Head Block Size:"));
    _headBlockSizePrefixLabel->setFont (Font (15.0000f, Font::plain));
    _headBlockSizePrefixLabel->setJustificationType (Justification::centredLeft);
    _headBlockSizePrefixLabel->setEditable (false, false, false);
    _headBlockSizePrefixLabel->setColour (Label::textColourId, Colour (0xff202020));
    _headBlockSizePrefixLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _headBlockSizePrefixLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_headBlockSizeLabel = new Label (String(),
                                                        L"<Unknown>"));
    _headBlockSizeLabel->setFont (Font (15.0000f, Font::plain));
    _headBlockSizeLabel->setJustificationType (Justification::centredLeft);
    _headBlockSizeLabel->setEditable (false, false, false);
    _headBlockSizeLabel->setColour (Label::textColourId, Colour (0xff202020));
    _headBlockSizeLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _headBlockSizeLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_tailBlockSizePrefixLabel = new Label (String(),
                                                              L"Tail Block Size:"));
    _tailBlockSizePrefixLabel->setFont (Font (15.0000f, Font::plain));
    _tailBlockSizePrefixLabel->setJustificationType (Justification::centredLeft);
    _tailBlockSizePrefixLabel->setEditable (false, false, false);
    _tailBlockSizePrefixLabel->setColour (Label::textColourId, Colour (0xff202020));
    _tailBlockSizePrefixLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _tailBlockSizePrefixLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_tailBlockSizeLabel = new Label (String(),
                                                        L"<Unknown>"));
    _tailBlockSizeLabel->setFont (Font (15.0000f, Font::plain));
    _tailBlockSizeLabel->setJustificationType (Justification::centredLeft);
    _tailBlockSizeLabel->setEditable (false, false, false);
    _tailBlockSizeLabel->setColour (Label::textColourId, Colour (0xff202020));
    _tailBlockSizeLabel->setColour (TextEditor::textColourId, Colour (0xff202020));
    _tailBlockSizeLabel->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (_selectIRDirectoryButton = new TextButton (String()));
    _selectIRDirectoryButton->setButtonText (L"Select Directory");
    _selectIRDirectoryButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    _selectIRDirectoryButton->addListener (this);

    cachedImage_hifilofi_jpg = ImageCache::getFromMemory (hifilofi_jpg, hifilofi_jpgSize);

    //[UserPreSize]
    const juce::File irDirectory = _processor.getSettings().getImpulseResponseDirectory();
    _irDirectoryBrowserComponent = new juce::FileBrowserComponent(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories | juce::FileBrowserComponent::filenameBoxIsReadOnly,
                                                                  irDirectory,
                                                                  nullptr,
                                                                  nullptr);
    _irDirectoryBrowserComponent->setFilenameBoxLabel(juce::String("Folder:"));
    _irDirectoryBrowserComponent->setFileName(irDirectory.getFullPathName());
    _irDirectoryGroupComponent->addAndMakeVisible(_irDirectoryBrowserComponent);
    //[/UserPreSize]

    setSize (504, 580);


    //[Constructor] You can add your own custom stuff here..
    _nameVersionLabel->setText(ProjectInfo::projectName + juce::String(" - Version ") + ProjectInfo::versionString, juce::sendNotification);
    _juceVersionLabel->setText(juce::SystemStats::getJUCEVersion(), juce::sendNotification);
    _numberInputsLabel->setText(juce::String(_processor.getTotalNumInputChannels()), juce::sendNotification);
    _numberOutputsLabel->setText(juce::String(_processor.getTotalNumOutputChannels()), juce::sendNotification);
    _sseOptimizationLabel->setText((fftconvolver::SSEEnabled() == true) ? juce::String("Yes") : juce::String("No"), juce::sendNotification);
    _headBlockSizeLabel->setText(juce::String(static_cast<int>(_processor.getConvolverHeadBlockSize())), juce::sendNotification);
    _tailBlockSizeLabel->setText(juce::String(static_cast<int>(_processor.getConvolverTailBlockSize())), juce::sendNotification);
    //[/Constructor]
}

SettingsDialogComponent::~SettingsDialogComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    deleteAndZero (_irDirectoryGroupComponent);
    deleteAndZero (_aboutGroupComponent);
    deleteAndZero (_nameVersionLabel);
    deleteAndZero (_copyrightLabel);
    deleteAndZero (_myLabel);
    deleteAndZero (_licenseHyperlink);
    deleteAndZero (_infoGroupComponent);
    deleteAndZero (_juceVersionPrefixLabel);
    deleteAndZero (_juceVersionLabel);
    deleteAndZero (_numberInputsPrefixLabel);
    deleteAndZero (_numberInputsLabel);
    deleteAndZero (_numberOutputsPrefixLabel);
    deleteAndZero (_numberOutputsLabel);
    deleteAndZero (_sseOptimizationPrefixLabel);
    deleteAndZero (_sseOptimizationLabel);
    deleteAndZero (_headBlockSizePrefixLabel);
    deleteAndZero (_headBlockSizeLabel);
    deleteAndZero (_tailBlockSizePrefixLabel);
    deleteAndZero (_tailBlockSizeLabel);
    deleteAndZero (_selectIRDirectoryButton);


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void SettingsDialogComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xffb1b1b6));

    g.setColour (Colours::black);
    g.drawImageWithin (cachedImage_hifilofi_jpg,
                       400, 31, 74, 69,
                       RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize,
                       false);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void SettingsDialogComponent::resized()
{
    _irDirectoryGroupComponent->setBounds (16, 120, 472, 288);
    _aboutGroupComponent->setBounds (16, 11, 472, 101);
    _nameVersionLabel->setBounds (24, 28, 344, 24);
    _copyrightLabel->setBounds (24, 52, 344, 24);
    _myLabel->setBounds (24, 76, 344, 24);
    _licenseHyperlink->setBounds (160, 76, 184, 24);
    _infoGroupComponent->setBounds (16, 416, 472, 152);
    _juceVersionPrefixLabel->setBounds (24, 436, 140, 24);
    _juceVersionLabel->setBounds (156, 436, 316, 24);
    _numberInputsPrefixLabel->setBounds (24, 456, 140, 24);
    _numberInputsLabel->setBounds (156, 456, 316, 24);
    _numberOutputsPrefixLabel->setBounds (24, 476, 140, 24);
    _numberOutputsLabel->setBounds (156, 476, 316, 24);
    _sseOptimizationPrefixLabel->setBounds (24, 496, 140, 24);
    _sseOptimizationLabel->setBounds (156, 496, 316, 24);
    _headBlockSizePrefixLabel->setBounds (24, 516, 140, 24);
    _headBlockSizeLabel->setBounds (156, 516, 316, 24);
    _tailBlockSizePrefixLabel->setBounds (24, 536, 140, 24);
    _tailBlockSizeLabel->setBounds (156, 536, 316, 24);
    _selectIRDirectoryButton->setBounds (352, 372, 124, 24);
    //[UserResized] Add your own custom resize handling here..
    _irDirectoryBrowserComponent->setBounds(4, 12, _irDirectoryGroupComponent->getWidth()-8, _irDirectoryGroupComponent->getHeight()-(_selectIRDirectoryButton->getHeight()+26));
    //[/UserResized]
}

void SettingsDialogComponent::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == _selectIRDirectoryButton)
    {
        //[UserButtonCode__selectIRDirectoryButton] -- add your button handler code here..
        if (_irDirectoryBrowserComponent && _irDirectoryBrowserComponent->getNumSelectedFiles() == 1)
        {
          const juce::File directory = _irDirectoryBrowserComponent->getSelectedFile(0);
          if (directory.exists() && directory.isDirectory())
          {
            _processor.getSettings().setImpulseResponseDirectory(directory);
            _irDirectoryBrowserComponent->setFileName(directory.getFullPathName());
          }
        }
        //[/UserButtonCode__selectIRDirectoryButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="SettingsDialogComponent"
                 componentName="" parentClasses="public Component" constructorParams="Processor&amp; processor"
                 variableInitialisers="_processor(processor)" snapPixels="4" snapActive="1"
                 snapShown="1" overlayOpacity="0.330000013" fixedSize="1" initialWidth="504"
                 initialHeight="580">
  <BACKGROUND backgroundColour="ffb1b1b6">
    <IMAGE pos="400 31 74 69" resource="hifilofi_jpg" opacity="1" mode="2"/>
  </BACKGROUND>
  <GROUPCOMPONENT name="" id="54a84aa39bb27f4b" memberName="_irDirectoryGroupComponent"
                  virtualName="" explicitFocusOrder="0" pos="16 120 472 288" textcol="ff202020"
                  title="Impulse Response Directory"/>
  <GROUPCOMPONENT name="" id="81749f34274f2c9a" memberName="_aboutGroupComponent"
                  virtualName="" explicitFocusOrder="0" pos="16 11 472 101" textcol="ff202020"
                  title="About"/>
  <LABEL name="" id="4b7ca86a8e675cd7" memberName="_nameVersionLabel"
         virtualName="" explicitFocusOrder="0" pos="24 28 344 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="KlangFalter - Version &lt;Unknown&gt;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="b963eaf05cdcbe30" memberName="_copyrightLabel" virtualName=""
         explicitFocusOrder="0" pos="24 52 344 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Copyright (c) 2013 HiFi-LoFi"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <HYPERLINKBUTTON name="" id="605db3d7e5dbbec" memberName="_licenseHyperlink" virtualName=""
                   explicitFocusOrder="0" pos="160 76 184 24" tooltip="http://www.gnu.org/licenses"
                   buttonText="Licensed under GPL3" connectedEdges="0" needsCallback="0"
                   radioGroupId="0" url="http://www.gnu.org/licenses"/>
  <GROUPCOMPONENT name="" id="25ac040a541cb0e7" memberName="_infoGroupComponent"
                  virtualName="" explicitFocusOrder="0" pos="16 416 472 152" textcol="ff202020"
                  title="Plugin Information"/>
  <LABEL name="" id="c4a4ccf3c53f694f" memberName="_juceVersionPrefixLabel"
         virtualName="" explicitFocusOrder="0" pos="24 436 140 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="JUCE Version:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="e47fab678f6315f0" memberName="_juceVersionLabel"
         virtualName="" explicitFocusOrder="0" pos="156 436 316 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="&lt;Unknown&gt;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="b930280e91f83049" memberName="_numberInputsPrefixLabel"
         virtualName="" explicitFocusOrder="0" pos="24 456 140 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Input Channels:"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="26b792a855f83bf7" memberName="_numberInputsLabel"
         virtualName="" explicitFocusOrder="0" pos="156 456 316 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="&lt;Unknown&gt;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="7ecaf5ecb63407f4" memberName="_numberOutputsPrefixLabel"
         virtualName="" explicitFocusOrder="0" pos="24 476 140 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Output Channels:"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="69f8c3850d35c1bd" memberName="_numberOutputsLabel"
         virtualName="" explicitFocusOrder="0" pos="156 476 316 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="&lt;Unknown&gt;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="ad7d4e6a39bb8ab5" memberName="_sseOptimizationPrefixLabel"
         virtualName="" explicitFocusOrder="0" pos="24 496 140 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="SSE Optimization:"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="7a6359296851c399" memberName="_sseOptimizationLabel"
         virtualName="" explicitFocusOrder="0" pos="156 496 316 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="&lt;Unknown&gt;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="b9baa71c9fe56254" memberName="_headBlockSizePrefixLabel"
         virtualName="" explicitFocusOrder="0" pos="24 516 140 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Head Block Size:"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="4156f958d766b1d1" memberName="_headBlockSizeLabel"
         virtualName="" explicitFocusOrder="0" pos="156 516 316 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="&lt;Unknown&gt;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="a82e3cf23273bc55" memberName="_tailBlockSizePrefixLabel"
         virtualName="" explicitFocusOrder="0" pos="24 536 140 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="Tail Block Size:"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="" id="9fa7e63d5dcac636" memberName="_tailBlockSizeLabel"
         virtualName="" explicitFocusOrder="0" pos="156 536 316 24" textCol="ff202020"
         edTextCol="ff202020" edBkgCol="0" labelText="&lt;Unknown&gt;"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <TEXTBUTTON name="" id="12129938a2f63765" memberName="_selectIRDirectoryButton"
              virtualName="" explicitFocusOrder="0" pos="352 372 124 24" buttonText="Select Directory"
              connectedEdges="3" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: hifilofi_jpg, 7971, "Resources/hifi-lofi.jpg"
static const unsigned char resource_SettingsDialogComponent_hifilofi_jpg[] = { 255,216,255,224,0,16,74,70,73,70,0,1,1,1,0,72,0,72,0,0,255,225,0,176,69,120,105,102,0,0,73,73,42,0,8,0,0,0,5,0,26,1,5,0,1,
0,0,0,74,0,0,0,27,1,5,0,1,0,0,0,82,0,0,0,40,1,3,0,1,0,0,0,2,0,0,0,49,1,2,0,12,0,0,0,90,0,0,0,105,135,4,0,1,0,0,0,102,0,0,0,0,0,0,0,72,0,0,0,1,0,0,0,72,0,0,0,1,0,0,0,71,73,77,80,32,50,46,54,46,49,48,0,
5,0,0,144,7,0,4,0,0,0,48,50,50,48,0,160,7,0,4,0,0,0,48,49,48,48,1,160,3,0,1,0,0,0,255,255,0,0,2,160,4,0,1,0,0,0,100,0,0,0,3,160,4,0,1,0,0,0,100,0,0,0,0,0,0,0,255,226,12,88,73,67,67,95,80,82,79,70,73,76,
69,0,1,1,0,0,12,72,76,105,110,111,2,16,0,0,109,110,116,114,82,71,66,32,88,89,90,32,7,206,0,2,0,9,0,6,0,49,0,0,97,99,115,112,77,83,70,84,0,0,0,0,73,69,67,32,115,82,71,66,0,0,0,0,0,0,0,0,0,0,0,0,0,0,246,
214,0,1,0,0,0,0,211,45,72,80,32,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,99,112,114,116,0,0,1,80,0,0,0,51,100,101,115,99,0,0,1,132,0,0,0,108,
119,116,112,116,0,0,1,240,0,0,0,20,98,107,112,116,0,0,2,4,0,0,0,20,114,88,89,90,0,0,2,24,0,0,0,20,103,88,89,90,0,0,2,44,0,0,0,20,98,88,89,90,0,0,2,64,0,0,0,20,100,109,110,100,0,0,2,84,0,0,0,112,100,109,
100,100,0,0,2,196,0,0,0,136,118,117,101,100,0,0,3,76,0,0,0,134,118,105,101,119,0,0,3,212,0,0,0,36,108,117,109,105,0,0,3,248,0,0,0,20,109,101,97,115,0,0,4,12,0,0,0,36,116,101,99,104,0,0,4,48,0,0,0,12,114,
84,82,67,0,0,4,60,0,0,8,12,103,84,82,67,0,0,4,60,0,0,8,12,98,84,82,67,0,0,4,60,0,0,8,12,116,101,120,116,0,0,0,0,67,111,112,121,114,105,103,104,116,32,40,99,41,32,49,57,57,56,32,72,101,119,108,101,116,
116,45,80,97,99,107,97,114,100,32,67,111,109,112,97,110,121,0,0,100,101,115,99,0,0,0,0,0,0,0,18,115,82,71,66,32,73,69,67,54,49,57,54,54,45,50,46,49,0,0,0,0,0,0,0,0,0,0,0,18,115,82,71,66,32,73,69,67,54,
49,57,54,54,45,50,46,49,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,88,89,90,32,0,0,0,0,0,0,243,81,0,1,0,0,0,1,22,204,88,89,90,32,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,88,89,90,32,0,0,0,0,0,0,111,162,0,0,56,245,0,0,3,144,88,89,90,32,0,0,0,0,0,0,98,153,0,0,183,133,0,0,24,218,88,89,90,32,0,0,0,0,0,0,36,160,0,0,15,132,0,0,182,207,100,101,115,99,0,0,0,
0,0,0,0,22,73,69,67,32,104,116,116,112,58,47,47,119,119,119,46,105,101,99,46,99,104,0,0,0,0,0,0,0,0,0,0,0,22,73,69,67,32,104,116,116,112,58,47,47,119,119,119,46,105,101,99,46,99,104,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100,101,115,99,0,0,0,0,0,0,0,46,73,69,67,32,54,49,57,54,54,45,50,46,49,32,68,101,102,97,117,108,116,32,82,71,66,32,99,111,108,
111,117,114,32,115,112,97,99,101,32,45,32,115,82,71,66,0,0,0,0,0,0,0,0,0,0,0,46,73,69,67,32,54,49,57,54,54,45,50,46,49,32,68,101,102,97,117,108,116,32,82,71,66,32,99,111,108,111,117,114,32,115,112,97,
99,101,32,45,32,115,82,71,66,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100,101,115,99,0,0,0,0,0,0,0,44,82,101,102,101,114,101,110,99,101,32,86,105,101,119,105,110,103,32,67,111,110,100,105,116,105,111,
110,32,105,110,32,73,69,67,54,49,57,54,54,45,50,46,49,0,0,0,0,0,0,0,0,0,0,0,44,82,101,102,101,114,101,110,99,101,32,86,105,101,119,105,110,103,32,67,111,110,100,105,116,105,111,110,32,105,110,32,73,69,
67,54,49,57,54,54,45,50,46,49,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,118,105,101,119,0,0,0,0,0,19,164,254,0,20,95,46,0,16,207,20,0,3,237,204,0,4,19,11,0,3,92,158,0,0,0,1,88,89,90,32,0,0,0,
0,0,76,9,86,0,80,0,0,0,87,31,231,109,101,97,115,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,143,0,0,0,2,115,105,103,32,0,0,0,0,67,82,84,32,99,117,114,118,0,0,0,0,0,0,4,0,0,0,0,5,0,10,0,15,0,
20,0,25,0,30,0,35,0,40,0,45,0,50,0,55,0,59,0,64,0,69,0,74,0,79,0,84,0,89,0,94,0,99,0,104,0,109,0,114,0,119,0,124,0,129,0,134,0,139,0,144,0,149,0,154,0,159,0,164,0,169,0,174,0,178,0,183,0,188,0,193,0,198,
0,203,0,208,0,213,0,219,0,224,0,229,0,235,0,240,0,246,0,251,1,1,1,7,1,13,1,19,1,25,1,31,1,37,1,43,1,50,1,56,1,62,1,69,1,76,1,82,1,89,1,96,1,103,1,110,1,117,1,124,1,131,1,139,1,146,1,154,1,161,1,169,1,
177,1,185,1,193,1,201,1,209,1,217,1,225,1,233,1,242,1,250,2,3,2,12,2,20,2,29,2,38,2,47,2,56,2,65,2,75,2,84,2,93,2,103,2,113,2,122,2,132,2,142,2,152,2,162,2,172,2,182,2,193,2,203,2,213,2,224,2,235,2,245,
3,0,3,11,3,22,3,33,3,45,3,56,3,67,3,79,3,90,3,102,3,114,3,126,3,138,3,150,3,162,3,174,3,186,3,199,3,211,3,224,3,236,3,249,4,6,4,19,4,32,4,45,4,59,4,72,4,85,4,99,4,113,4,126,4,140,4,154,4,168,4,182,4,196,
4,211,4,225,4,240,4,254,5,13,5,28,5,43,5,58,5,73,5,88,5,103,5,119,5,134,5,150,5,166,5,181,5,197,5,213,5,229,5,246,6,6,6,22,6,39,6,55,6,72,6,89,6,106,6,123,6,140,6,157,6,175,6,192,6,209,6,227,6,245,7,7,
7,25,7,43,7,61,7,79,7,97,7,116,7,134,7,153,7,172,7,191,7,210,7,229,7,248,8,11,8,31,8,50,8,70,8,90,8,110,8,130,8,150,8,170,8,190,8,210,8,231,8,251,9,16,9,37,9,58,9,79,9,100,9,121,9,143,9,164,9,186,9,207,
9,229,9,251,10,17,10,39,10,61,10,84,10,106,10,129,10,152,10,174,10,197,10,220,10,243,11,11,11,34,11,57,11,81,11,105,11,128,11,152,11,176,11,200,11,225,11,249,12,18,12,42,12,67,12,92,12,117,12,142,12,167,
12,192,12,217,12,243,13,13,13,38,13,64,13,90,13,116,13,142,13,169,13,195,13,222,13,248,14,19,14,46,14,73,14,100,14,127,14,155,14,182,14,210,14,238,15,9,15,37,15,65,15,94,15,122,15,150,15,179,15,207,15,
236,16,9,16,38,16,67,16,97,16,126,16,155,16,185,16,215,16,245,17,19,17,49,17,79,17,109,17,140,17,170,17,201,17,232,18,7,18,38,18,69,18,100,18,132,18,163,18,195,18,227,19,3,19,35,19,67,19,99,19,131,19,
164,19,197,19,229,20,6,20,39,20,73,20,106,20,139,20,173,20,206,20,240,21,18,21,52,21,86,21,120,21,155,21,189,21,224,22,3,22,38,22,73,22,108,22,143,22,178,22,214,22,250,23,29,23,65,23,101,23,137,23,174,
23,210,23,247,24,27,24,64,24,101,24,138,24,175,24,213,24,250,25,32,25,69,25,107,25,145,25,183,25,221,26,4,26,42,26,81,26,119,26,158,26,197,26,236,27,20,27,59,27,99,27,138,27,178,27,218,28,2,28,42,28,82,
28,123,28,163,28,204,28,245,29,30,29,71,29,112,29,153,29,195,29,236,30,22,30,64,30,106,30,148,30,190,30,233,31,19,31,62,31,105,31,148,31,191,31,234,32,21,32,65,32,108,32,152,32,196,32,240,33,28,33,72,
33,117,33,161,33,206,33,251,34,39,34,85,34,130,34,175,34,221,35,10,35,56,35,102,35,148,35,194,35,240,36,31,36,77,36,124,36,171,36,218,37,9,37,56,37,104,37,151,37,199,37,247,38,39,38,87,38,135,38,183,38,
232,39,24,39,73,39,122,39,171,39,220,40,13,40,63,40,113,40,162,40,212,41,6,41,56,41,107,41,157,41,208,42,2,42,53,42,104,42,155,42,207,43,2,43,54,43,105,43,157,43,209,44,5,44,57,44,110,44,162,44,215,45,
12,45,65,45,118,45,171,45,225,46,22,46,76,46,130,46,183,46,238,47,36,47,90,47,145,47,199,47,254,48,53,48,108,48,164,48,219,49,18,49,74,49,130,49,186,49,242,50,42,50,99,50,155,50,212,51,13,51,70,51,127,
51,184,51,241,52,43,52,101,52,158,52,216,53,19,53,77,53,135,53,194,53,253,54,55,54,114,54,174,54,233,55,36,55,96,55,156,55,215,56,20,56,80,56,140,56,200,57,5,57,66,57,127,57,188,57,249,58,54,58,116,58,
178,58,239,59,45,59,107,59,170,59,232,60,39,60,101,60,164,60,227,61,34,61,97,61,161,61,224,62,32,62,96,62,160,62,224,63,33,63,97,63,162,63,226,64,35,64,100,64,166,64,231,65,41,65,106,65,172,65,238,66,
48,66,114,66,181,66,247,67,58,67,125,67,192,68,3,68,71,68,138,68,206,69,18,69,85,69,154,69,222,70,34,70,103,70,171,70,240,71,53,71,123,71,192,72,5,72,75,72,145,72,215,73,29,73,99,73,169,73,240,74,55,74,
125,74,196,75,12,75,83,75,154,75,226,76,42,76,114,76,186,77,2,77,74,77,147,77,220,78,37,78,110,78,183,79,0,79,73,79,147,79,221,80,39,80,113,80,187,81,6,81,80,81,155,81,230,82,49,82,124,82,199,83,19,83,
95,83,170,83,246,84,66,84,143,84,219,85,40,85,117,85,194,86,15,86,92,86,169,86,247,87,68,87,146,87,224,88,47,88,125,88,203,89,26,89,105,89,184,90,7,90,86,90,166,90,245,91,69,91,149,91,229,92,53,92,134,
92,214,93,39,93,120,93,201,94,26,94,108,94,189,95,15,95,97,95,179,96,5,96,87,96,170,96,252,97,79,97,162,97,245,98,73,98,156,98,240,99,67,99,151,99,235,100,64,100,148,100,233,101,61,101,146,101,231,102,
61,102,146,102,232,103,61,103,147,103,233,104,63,104,150,104,236,105,67,105,154,105,241,106,72,106,159,106,247,107,79,107,167,107,255,108,87,108,175,109,8,109,96,109,185,110,18,110,107,110,196,111,30,
111,120,111,209,112,43,112,134,112,224,113,58,113,149,113,240,114,75,114,166,115,1,115,93,115,184,116,20,116,112,116,204,117,40,117,133,117,225,118,62,118,155,118,248,119,86,119,179,120,17,120,110,120,
204,121,42,121,137,121,231,122,70,122,165,123,4,123,99,123,194,124,33,124,129,124,225,125,65,125,161,126,1,126,98,126,194,127,35,127,132,127,229,128,71,128,168,129,10,129,107,129,205,130,48,130,146,130,
244,131,87,131,186,132,29,132,128,132,227,133,71,133,171,134,14,134,114,134,215,135,59,135,159,136,4,136,105,136,206,137,51,137,153,137,254,138,100,138,202,139,48,139,150,139,252,140,99,140,202,141,49,
141,152,141,255,142,102,142,206,143,54,143,158,144,6,144,110,144,214,145,63,145,168,146,17,146,122,146,227,147,77,147,182,148,32,148,138,148,244,149,95,149,201,150,52,150,159,151,10,151,117,151,224,152,
76,152,184,153,36,153,144,153,252,154,104,154,213,155,66,155,175,156,28,156,137,156,247,157,100,157,210,158,64,158,174,159,29,159,139,159,250,160,105,160,216,161,71,161,182,162,38,162,150,163,6,163,118,
163,230,164,86,164,199,165,56,165,169,166,26,166,139,166,253,167,110,167,224,168,82,168,196,169,55,169,169,170,28,170,143,171,2,171,117,171,233,172,92,172,208,173,68,173,184,174,45,174,161,175,22,175,
139,176,0,176,117,176,234,177,96,177,214,178,75,178,194,179,56,179,174,180,37,180,156,181,19,181,138,182,1,182,121,182,240,183,104,183,224,184,89,184,209,185,74,185,194,186,59,186,181,187,46,187,167,188,
33,188,155,189,21,189,143,190,10,190,132,190,255,191,122,191,245,192,112,192,236,193,103,193,227,194,95,194,219,195,88,195,212,196,81,196,206,197,75,197,200,198,70,198,195,199,65,199,191,200,61,200,188,
201,58,201,185,202,56,202,183,203,54,203,182,204,53,204,181,205,53,205,181,206,54,206,182,207,55,207,184,208,57,208,186,209,60,209,190,210,63,210,193,211,68,211,198,212,73,212,203,213,78,213,209,214,85,
214,216,215,92,215,224,216,100,216,232,217,108,217,241,218,118,218,251,219,128,220,5,220,138,221,16,221,150,222,28,222,162,223,41,223,175,224,54,224,189,225,68,225,204,226,83,226,219,227,99,227,235,228,
115,228,252,229,132,230,13,230,150,231,31,231,169,232,50,232,188,233,70,233,208,234,91,234,229,235,112,235,251,236,134,237,17,237,156,238,40,238,180,239,64,239,204,240,88,240,229,241,114,241,255,242,140,
243,25,243,167,244,52,244,194,245,80,245,222,246,109,246,251,247,138,248,25,248,168,249,56,249,199,250,87,250,231,251,119,252,7,252,152,253,41,253,186,254,75,254,220,255,109,255,255,255,219,0,67,0,7,5,
5,6,5,4,7,6,6,6,8,7,7,8,11,18,11,11,10,10,11,22,15,16,13,18,26,22,27,26,25,22,25,24,28,32,40,34,28,30,38,30,24,25,35,48,36,38,42,43,45,46,45,27,34,50,53,49,44,53,40,44,45,44,255,219,0,67,1,7,8,8,11,9,
11,21,11,11,21,44,29,25,29,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,255,192,0,17,8,0,100,0,
100,3,1,17,0,2,17,1,3,17,1,255,196,0,28,0,0,0,7,1,1,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,0,8,255,196,0,64,16,0,1,3,3,3,1,4,7,4,8,4,7,0,0,0,0,1,2,3,4,0,5,17,6,18,33,49,7,19,65,81,20,34,50,97,113,129,145,
66,146,161,209,21,36,51,82,162,177,193,241,22,130,178,225,8,68,83,98,116,210,240,255,196,0,27,1,0,2,3,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,255,196,0,52,17,0,1,4,1,2,4,3,7,3,3,5,0,0,0,0,0,1,0,2,3,
17,4,18,33,5,19,49,81,65,113,177,20,34,50,66,97,129,145,161,209,225,21,35,51,22,36,52,114,193,255,218,0,12,3,1,0,2,17,3,17,0,63,0,209,123,162,8,0,168,131,226,85,154,206,10,232,144,129,69,32,4,40,237,231,
141,220,138,66,211,71,219,147,145,181,36,120,128,42,91,165,98,146,75,86,0,27,143,158,20,63,42,18,222,210,1,37,47,231,189,80,90,122,30,71,247,165,183,116,235,110,136,225,197,173,32,16,174,189,114,70,124,
255,0,189,0,38,74,75,32,18,73,94,223,251,145,129,244,164,149,164,10,54,39,0,103,42,30,24,25,252,233,162,209,65,94,243,140,36,121,227,140,252,252,40,39,186,60,150,115,218,110,154,155,115,159,107,48,153,
72,109,213,6,85,176,0,55,168,240,79,192,84,163,32,42,230,105,117,42,14,179,143,34,217,117,77,173,249,72,127,209,208,159,217,160,165,41,227,24,7,169,233,214,173,105,189,213,18,109,178,175,188,158,237,13,
16,242,87,189,59,176,156,250,190,227,239,169,170,233,26,52,37,73,108,175,188,198,14,57,164,81,86,189,110,30,142,190,18,243,124,227,144,161,227,211,199,199,241,172,212,71,130,223,127,84,77,237,227,57,4,
117,202,84,60,60,122,248,82,164,237,112,8,74,149,235,0,125,159,107,60,249,83,164,173,112,81,88,29,217,220,56,251,39,235,210,157,82,68,222,233,49,28,169,194,61,98,79,76,146,79,251,82,77,114,144,27,78,28,
220,48,50,2,134,127,181,47,52,252,145,22,164,228,130,10,8,253,227,200,197,52,146,28,4,164,133,171,146,72,10,86,115,154,87,73,213,238,138,81,176,29,203,81,80,253,252,19,79,116,182,76,47,5,66,201,37,196,
72,12,169,166,84,164,186,78,54,16,147,207,255,0,117,165,208,167,177,5,121,166,68,167,164,60,167,31,144,227,170,81,229,107,81,36,251,249,173,97,115,202,68,96,44,29,216,72,61,79,95,165,8,70,110,90,144,21,
149,44,149,18,73,10,197,36,90,110,153,111,54,172,165,247,82,65,7,133,17,200,233,227,225,80,79,100,62,159,36,163,111,164,190,83,130,156,111,56,193,57,35,175,137,230,132,108,133,51,230,169,71,100,167,247,
100,171,61,225,235,140,103,227,142,40,78,210,141,94,110,76,99,186,184,202,108,140,15,85,229,14,157,60,124,40,66,89,58,150,242,132,144,155,188,192,8,32,254,176,190,135,175,143,141,20,139,61,211,132,107,
125,68,209,202,111,179,1,221,191,246,228,243,140,103,233,69,35,81,238,157,69,214,154,181,210,27,141,116,184,63,194,70,212,169,75,232,114,60,63,189,4,119,76,56,248,43,12,11,143,106,211,155,9,139,30,246,
226,48,70,75,5,35,7,147,202,128,255,0,106,161,242,196,207,137,192,125,213,131,152,122,90,156,131,110,237,138,66,84,178,131,29,57,201,50,221,105,177,211,30,62,31,214,178,201,196,49,35,217,207,30,190,138,
209,20,221,144,220,32,107,55,109,82,33,222,181,142,156,138,195,141,236,91,110,74,66,148,64,231,30,168,206,79,137,168,14,35,19,141,70,215,59,200,20,249,114,29,156,224,22,72,169,202,36,229,164,249,112,43,
172,177,34,122,80,61,90,7,231,69,148,173,0,146,159,250,67,230,104,180,148,138,244,158,166,79,42,211,215,97,241,136,191,202,169,230,176,244,112,87,104,119,101,45,166,59,62,189,223,175,109,192,150,196,171,
59,10,4,169,249,44,20,142,157,18,147,141,202,62,66,179,228,229,183,30,51,39,85,100,112,151,187,73,217,78,199,208,26,114,211,114,12,94,53,220,52,188,135,0,84,86,34,184,227,135,159,103,28,114,107,56,207,
153,212,99,128,159,184,10,78,133,173,37,174,112,77,215,15,178,200,45,151,23,119,212,55,53,131,130,26,140,150,144,79,150,79,231,64,151,57,231,252,109,111,153,191,68,6,194,62,34,74,5,106,190,205,162,39,
245,93,17,50,114,199,59,166,77,35,63,33,154,124,172,231,29,228,104,31,65,126,169,234,132,31,134,208,14,213,96,196,80,54,205,11,96,138,19,236,151,91,47,17,243,52,253,142,98,61,249,157,246,160,142,123,71,
70,5,207,118,225,172,251,189,177,100,66,183,180,122,38,52,68,39,21,80,225,24,231,227,46,119,155,138,143,61,195,160,80,147,187,76,214,55,16,68,157,73,60,164,248,33,221,131,248,113,90,25,195,113,25,210,
49,235,234,163,207,147,192,168,9,87,89,211,85,186,92,249,82,15,155,175,41,95,204,214,198,68,198,124,45,3,236,170,47,113,234,83,67,176,249,85,170,11,178,60,232,66,12,143,58,18,93,159,121,161,11,209,183,
93,61,218,101,182,207,46,224,238,182,140,226,33,180,183,202,3,60,157,163,36,12,167,221,94,26,14,33,129,35,196,98,34,44,129,215,249,93,242,37,27,234,253,19,221,77,165,245,30,181,141,25,235,125,209,152,
209,82,203,107,84,105,108,239,220,233,27,183,2,1,199,7,21,216,225,216,207,118,161,27,53,81,32,144,234,245,85,102,100,178,42,15,61,126,150,160,31,147,171,108,10,65,213,58,34,45,246,59,65,33,18,97,160,41,
77,132,140,2,54,131,140,117,232,42,177,151,12,206,172,121,180,56,30,135,249,75,70,222,251,111,201,86,255,0,66,118,101,169,219,44,219,239,51,116,196,181,28,152,243,6,246,183,115,231,241,63,106,181,156,
156,184,77,201,30,161,221,191,178,164,227,198,227,77,117,121,168,171,215,98,250,154,218,215,164,192,49,111,113,8,220,151,97,57,146,71,158,211,138,186,14,41,143,49,211,116,123,29,149,46,198,145,190,22,
168,147,33,74,183,200,44,77,140,244,87,71,84,60,130,133,125,13,116,174,250,44,231,110,169,3,156,1,224,41,164,130,132,151,117,161,36,20,208,187,194,132,144,80,133,212,36,189,163,171,56,209,119,174,63,228,
158,255,0,65,175,144,97,31,247,49,249,143,85,233,202,90,198,146,139,106,61,176,54,182,70,227,158,168,29,61,213,171,58,67,204,26,77,117,233,230,83,118,229,73,133,4,140,242,43,0,22,163,74,26,247,164,52,
254,163,65,23,75,76,89,42,35,246,133,27,86,63,204,48,107,100,25,147,227,255,0,137,228,122,126,20,92,198,187,168,84,73,125,142,72,180,60,100,232,221,75,54,210,239,94,229,213,149,54,126,99,250,131,93,150,
113,182,202,52,230,70,28,59,142,170,190,73,31,1,165,19,62,233,175,236,241,204,125,83,165,34,106,104,61,11,173,180,22,79,191,128,127,144,53,211,130,92,89,63,227,76,88,123,31,231,255,0,10,139,181,116,145,
183,228,171,142,195,236,167,84,172,161,43,157,164,103,147,130,149,141,205,103,224,114,7,212,86,241,62,124,59,189,130,70,247,111,85,148,193,19,190,19,94,105,141,199,176,235,240,103,210,172,19,160,106,8,
167,162,163,58,18,191,186,78,62,132,213,145,113,140,103,157,47,37,135,177,217,80,252,89,27,245,9,216,236,78,110,156,181,127,137,181,67,205,170,199,25,40,113,230,24,89,67,238,21,28,119,99,35,9,32,145,147,
244,174,160,120,45,212,221,213,6,51,122,111,117,43,107,159,217,3,78,184,243,150,22,215,199,13,62,183,84,7,227,214,161,173,202,173,46,9,13,79,164,123,55,212,150,105,154,142,193,127,111,79,46,59,32,170,
11,140,41,76,184,230,56,8,57,206,79,187,60,213,129,254,10,206,91,180,234,43,24,41,59,176,65,6,172,85,32,35,6,132,47,105,234,212,227,69,222,191,240,158,255,0,65,175,144,97,128,50,35,255,0,176,245,94,154,
237,56,179,168,155,83,10,35,25,105,191,159,168,41,230,10,127,220,250,171,29,213,62,200,34,178,180,210,138,14,84,177,131,128,58,251,234,192,69,36,185,94,208,162,246,82,8,190,201,224,226,149,218,125,84,
101,219,78,89,47,205,148,93,109,113,38,103,237,56,216,221,247,135,63,141,105,131,42,104,13,196,226,20,28,192,238,170,143,51,177,120,113,159,50,244,181,242,125,134,79,80,144,178,182,207,226,15,243,174,
211,56,227,158,52,101,48,60,42,121,58,119,97,165,23,114,187,118,145,163,173,143,255,0,136,33,91,117,37,148,126,217,197,148,227,30,106,7,31,138,77,110,197,246,44,151,233,196,123,163,119,111,15,199,69,84,
142,115,71,247,0,33,87,53,94,186,208,211,44,168,136,206,150,106,68,232,138,218,132,239,45,180,223,30,210,84,159,109,25,251,7,31,46,107,208,98,179,41,164,137,136,35,192,248,253,194,197,51,226,249,70,253,
146,221,151,216,226,235,187,77,233,187,252,128,168,239,148,69,67,109,182,18,99,41,35,40,117,191,120,206,54,248,140,231,53,184,251,181,75,157,52,206,46,22,178,221,83,96,147,165,181,52,219,44,151,219,144,
228,55,54,119,141,18,82,177,212,17,242,61,60,42,208,108,90,151,133,168,115,214,154,23,179,181,68,174,243,74,94,90,40,80,204,39,176,113,199,176,107,229,24,208,145,52,111,191,152,122,175,84,99,161,104,116,
196,179,54,193,29,101,27,84,150,219,73,194,130,178,118,15,46,159,10,124,82,14,76,218,110,238,207,234,150,171,59,169,108,154,230,13,147,217,60,133,110,122,106,20,182,138,64,74,182,157,223,12,215,99,7,133,
77,156,194,232,136,20,107,127,202,205,54,67,33,52,228,154,226,168,23,247,56,19,232,252,168,236,87,172,63,121,60,114,62,30,70,183,255,0,167,178,27,177,115,127,95,217,33,144,210,1,104,235,229,248,250,38,
183,183,5,138,218,153,242,130,215,29,71,27,153,65,88,30,89,242,7,192,213,115,112,9,224,110,183,188,87,223,246,90,48,221,237,146,114,99,30,247,212,128,134,222,166,110,150,133,92,225,73,105,248,232,59,85,
128,66,146,124,136,35,175,53,158,110,19,44,80,59,35,80,45,29,173,57,139,160,155,145,43,72,114,111,112,185,194,180,194,84,203,132,166,162,199,65,0,184,234,182,128,73,192,31,90,229,195,12,147,187,68,98,
202,78,112,104,178,188,251,218,38,168,186,107,29,102,237,154,12,200,206,198,136,255,0,117,21,182,14,230,220,86,50,84,73,224,171,130,50,120,30,30,117,244,110,15,130,204,72,67,234,156,238,183,215,203,201,
113,114,165,47,113,3,160,84,71,34,204,117,183,35,71,29,235,145,146,183,92,8,0,39,8,56,81,62,127,143,78,43,184,61,224,66,198,69,43,159,100,119,164,65,185,190,208,220,151,31,0,133,125,148,123,192,243,247,
213,248,216,194,103,123,221,2,203,62,194,209,59,102,101,180,222,109,83,27,8,75,242,35,40,56,83,246,128,87,4,253,72,249,85,249,140,13,112,3,178,49,220,72,54,179,3,156,214,21,161,109,119,62,221,30,185,91,
101,66,86,159,67,104,146,210,217,82,132,162,72,10,24,36,122,190,250,243,49,240,72,35,112,112,113,219,117,223,246,135,85,82,101,166,187,101,119,76,217,91,183,51,103,244,176,140,101,231,229,157,202,192,
192,232,159,32,42,236,190,19,14,83,245,188,215,146,131,102,32,87,85,61,19,183,187,164,233,45,69,137,165,89,125,247,148,16,219,104,125,106,82,137,240,3,21,129,220,3,21,163,83,156,64,10,66,82,74,213,97,
70,185,220,224,48,237,225,183,45,178,211,201,102,20,149,4,12,224,242,115,235,30,7,90,243,238,205,246,66,89,132,226,27,127,170,217,19,244,3,96,27,238,1,86,54,174,115,27,94,74,208,84,120,220,80,50,113,86,
14,61,153,168,184,17,103,232,177,28,72,136,170,77,218,239,24,134,236,64,174,242,59,165,69,77,58,55,167,10,234,144,15,65,238,160,113,204,214,183,69,130,62,161,90,230,53,239,18,116,112,173,198,221,60,124,
254,170,38,109,226,221,162,244,138,219,146,243,113,45,141,30,132,101,107,81,57,0,120,169,68,209,22,94,102,115,125,138,48,40,253,63,91,83,201,120,124,167,38,83,239,47,61,235,77,117,43,88,192,187,179,58,
40,106,40,13,63,1,0,254,196,37,68,40,159,50,160,112,126,2,189,207,14,225,241,112,248,180,183,119,30,167,191,236,23,26,89,31,144,226,71,65,186,205,163,41,97,69,8,60,43,168,174,137,11,40,42,106,205,49,171,
102,176,101,215,218,37,182,157,219,221,168,12,0,120,193,7,227,83,109,2,162,108,245,83,208,173,139,211,58,225,230,22,162,150,138,20,251,74,198,114,158,160,254,21,175,25,218,36,165,76,163,83,81,59,78,89,
94,161,183,187,222,184,176,228,97,144,163,157,163,113,232,60,176,106,121,195,251,163,201,71,28,157,37,81,230,37,9,150,226,90,81,83,97,68,32,159,17,225,88,13,120,45,46,187,247,186,175,83,30,202,180,57,
66,18,139,82,64,207,56,113,68,159,137,207,74,249,223,245,172,189,206,161,248,94,144,226,180,124,168,232,236,179,72,178,192,8,183,45,71,166,228,171,42,169,14,57,150,79,86,168,156,102,118,78,109,122,23,
75,197,116,174,53,190,99,50,17,144,29,202,155,87,200,143,141,91,47,20,201,35,119,54,143,106,41,8,128,59,43,4,43,12,72,135,8,114,82,128,32,128,185,43,88,252,77,115,165,206,116,140,173,35,240,20,234,183,
82,59,27,142,10,134,70,84,73,228,158,113,88,9,14,162,83,220,168,221,69,169,160,105,155,51,215,59,147,138,75,45,14,18,129,149,44,248,37,35,204,211,196,197,151,54,78,92,67,246,10,18,57,177,139,114,242,198,
182,215,87,93,113,125,68,169,96,181,29,165,126,171,21,42,245,27,25,252,84,124,77,125,51,3,135,197,131,30,136,250,248,158,235,137,52,198,83,103,162,107,233,178,45,112,46,93,211,104,66,174,13,152,235,5,
33,99,187,42,10,32,121,16,83,215,221,93,2,168,80,44,43,99,232,56,232,115,66,18,146,164,169,233,174,60,125,82,165,100,129,231,72,10,9,185,197,198,202,181,51,169,63,75,220,109,242,231,178,183,222,75,78,
67,90,26,33,37,123,189,147,207,30,61,50,51,87,49,244,224,74,173,194,194,146,215,246,215,81,99,176,220,59,247,30,105,180,24,193,78,50,26,82,82,6,224,133,12,158,65,10,31,42,215,148,219,107,94,168,132,209,
45,89,210,148,86,178,162,57,81,205,115,150,149,181,137,58,222,222,31,245,131,200,142,112,162,120,205,120,114,220,25,43,194,215,183,6,97,216,165,91,215,122,166,32,111,191,128,165,7,125,156,117,53,3,195,
241,93,101,175,232,142,99,254,102,41,24,221,174,76,101,208,137,80,92,70,222,22,72,60,84,93,194,26,225,109,117,170,139,216,77,57,180,172,214,238,213,109,82,86,2,220,8,39,207,138,194,254,27,51,7,68,249,
113,59,225,42,102,225,218,5,138,5,176,76,147,40,4,21,4,132,167,214,82,143,144,21,86,55,13,159,42,78,83,71,153,236,179,78,209,142,221,110,59,43,92,125,73,163,61,23,186,85,250,204,250,22,61,110,241,246,
206,239,136,38,190,139,133,133,22,20,92,168,135,153,238,87,157,150,87,74,109,202,69,219,22,158,159,13,8,93,158,219,34,51,131,114,63,87,108,165,64,248,130,7,226,43,110,202,149,3,114,236,159,68,221,20,28,
85,137,166,28,227,215,96,148,18,60,136,206,8,162,144,138,190,200,116,11,145,21,29,122,94,33,74,128,202,198,66,248,241,220,15,6,138,66,103,39,177,109,5,33,164,161,86,38,18,18,122,160,148,19,241,32,243,
74,144,177,30,221,244,101,147,68,200,176,71,178,48,182,83,36,58,181,133,58,165,251,37,56,3,36,227,169,166,6,233,20,234,203,108,26,179,76,185,30,226,220,249,113,48,30,71,163,181,189,224,160,60,18,72,206,
14,114,51,93,89,133,193,248,88,217,180,180,170,243,52,142,146,102,70,193,112,191,50,64,245,144,245,180,5,3,242,85,115,52,149,173,111,235,67,107,180,202,81,64,245,156,193,227,175,53,242,142,81,212,23,183,
213,239,4,71,160,197,93,206,26,84,210,118,161,188,142,40,17,188,52,210,98,67,69,65,77,210,246,169,140,78,148,182,212,86,165,108,24,81,0,123,240,43,161,12,210,176,6,5,7,209,59,170,126,180,211,250,107,76,
199,102,76,179,177,27,61,86,208,114,183,85,142,131,243,240,174,166,35,178,103,37,163,243,217,103,153,240,196,221,78,11,31,122,226,183,158,42,87,168,50,118,167,118,118,143,42,245,81,180,70,40,47,57,43,
204,174,183,37,226,69,155,55,62,139,2,68,140,117,238,153,82,241,244,21,110,162,170,45,11,114,208,154,131,181,171,12,8,86,68,105,7,166,196,104,158,237,82,218,40,82,81,140,236,11,200,0,15,12,252,40,178,
150,203,80,26,195,88,33,173,206,246,113,48,145,198,26,184,50,163,241,0,226,158,233,108,155,185,218,123,145,85,182,235,161,181,68,47,53,38,50,95,72,251,132,230,132,39,44,246,169,162,220,80,75,247,67,5,
103,236,77,140,235,10,254,36,211,180,44,131,254,33,238,86,29,78,254,155,106,197,115,106,229,116,10,113,176,196,69,119,128,161,120,218,73,28,3,184,99,30,243,229,69,238,162,80,118,93,19,81,196,191,136,77,
202,146,252,150,28,79,124,214,224,243,73,0,225,71,188,234,145,212,28,103,56,174,131,37,104,101,72,118,62,11,51,163,118,171,104,94,135,122,223,18,67,133,110,194,97,213,116,220,166,194,143,212,138,231,173,
42,171,250,5,167,35,22,76,185,91,92,59,143,174,50,15,187,138,231,142,31,140,62,69,179,218,230,235,169,28,233,134,94,154,149,170,225,60,22,211,180,97,196,244,251,180,198,6,56,27,49,68,229,204,62,100,138,
187,62,128,244,101,32,220,174,128,41,91,142,215,210,57,251,181,49,141,11,122,53,35,147,43,142,238,72,205,236,107,73,222,228,55,46,238,220,219,140,142,239,104,91,242,149,234,129,224,0,192,31,74,189,145,
178,49,77,20,160,233,28,243,110,41,196,126,206,180,197,154,41,93,182,213,30,50,219,60,43,186,67,135,248,193,169,208,85,151,20,233,155,180,230,39,181,13,18,84,26,233,194,82,159,228,0,252,41,210,6,234,95,
190,148,27,88,244,199,142,58,19,180,255,0,74,41,9,37,72,150,20,128,37,184,1,235,234,167,242,161,9,53,79,156,130,173,179,92,31,229,71,31,195,73,52,147,179,102,184,141,139,152,181,3,215,45,182,115,252,53,
32,145,81,242,108,182,249,203,253,106,28,87,136,228,19,25,160,160,124,194,130,114,15,192,211,34,212,87,91,109,145,109,46,60,245,189,164,196,113,121,42,45,36,12,252,70,48,104,160,17,106,81,185,211,11,96,
250,82,190,226,63,245,164,133,255,217,0,0};

const char* SettingsDialogComponent::hifilofi_jpg = (const char*) resource_SettingsDialogComponent_hifilofi_jpg;
const int SettingsDialogComponent::hifilofi_jpgSize = 7971;
