/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "AboutLayout.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
// TODO: add texts and hyperlinks
//[/MiscUserDefs]

//==============================================================================
AboutLayout::AboutLayout ()
{
    addAndMakeVisible (mLogoComponent = new ImageComponent());
    mLogoComponent->setName ("logoComonent");

    addAndMakeVisible (mNameLabel = new Label ("name label",
                                               TRANS("EasySSP v0.1")));
    mNameLabel->setFont (Font (15.00f, Font::bold));
    mNameLabel->setJustificationType (Justification::centredLeft);
    mNameLabel->setEditable (false, false, false);
    mNameLabel->setColour (TextEditor::textColourId, Colours::black);
    mNameLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mShortDescription = new Label ("short decription",
                                                      TRANS("Easy Sound Space Perception is a small and lightweight sound visualization tool.")));
    mShortDescription->setFont (Font (15.00f, Font::plain));
    mShortDescription->setJustificationType (Justification::centredLeft);
    mShortDescription->setEditable (false, false, false);
    mShortDescription->setColour (TextEditor::textColourId, Colours::black);
    mShortDescription->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mHomePageButton = new HyperlinkButton (TRANS("au.tomatl.org"),
                                                              URL ("http://au.tomatl.org/essp")));
    mHomePageButton->setTooltip (TRANS("http://au.tomatl.org/essp"));
    mHomePageButton->setButtonText (TRANS("au.tomatl.org"));

    addAndMakeVisible (mCopyrightLabel = new Label ("copyright label",
                                                    TRANS("(c) 2013-2014 Aleksandr Prokopchuk")));
    mCopyrightLabel->setFont (Font (15.00f, Font::plain));
    mCopyrightLabel->setJustificationType (Justification::centredLeft);
    mCopyrightLabel->setEditable (false, false, false);
    mCopyrightLabel->setColour (TextEditor::textColourId, Colours::black);
    mCopyrightLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (mGplNotice = new Label ("gpl notice",
                                               TRANS("This program comes with ABSOLUTELY NO WARRANTY;\n"
                                               "for details, visit http://www.gnu.org/licenses/gpl.html")));
    mGplNotice->setFont (Font (13.60f, Font::plain));
    mGplNotice->setJustificationType (Justification::centredLeft);
    mGplNotice->setEditable (false, false, false);
    mGplNotice->setColour (TextEditor::textColourId, Colours::black);
    mGplNotice->setColour (TextEditor::backgroundColourId, Colour (0x00000000));


    //[UserPreSize]
	// This hack unsets label colours assigned by Introjucer, as there is no way to avoid these colors automatic generation
	for (int i = 0; i < getNumChildComponents(); ++i)
	{
		Component* comp = getChildComponent(i);

		Label* label = NULL;
		label = dynamic_cast<Label*>(comp);

		if (label != NULL)
		{
			label->removeColour(TextEditor::textColourId);
			label->removeColour(TextEditor::backgroundColourId);
		}
	}
    //[/UserPreSize]

    setSize (350, 200);


    //[Constructor] You can add your own custom stuff here..
	mLogoComponent->setImage(ImageCache::getFromMemory(BinaryData::admvicon_png, BinaryData::admvicon_pngSize));

    //[/Constructor]
}

AboutLayout::~AboutLayout()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    mLogoComponent = nullptr;
    mNameLabel = nullptr;
    mShortDescription = nullptr;
    mHomePageButton = nullptr;
    mCopyrightLabel = nullptr;
    mGplNotice = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void AboutLayout::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::white);

    //[UserPaint] Add your own custom painting code here..
	g.fillAll(LookAndFeel::getDefaultLookAndFeel().findColour(TomatlLookAndFeel::defaultBackground));
    //[/UserPaint]
}

void AboutLayout::resized()
{
    mLogoComponent->setBounds (8, 8, 48, 48);
    mNameLabel->setBounds (56, 8, 150, 24);
    mShortDescription->setBounds (8, 64, 328, 48);
    mHomePageButton->setBounds (56, 32, 104, 24);
    mCopyrightLabel->setBounds (8, 120, 328, 24);
    mGplNotice->setBounds (8, 160, 384, 40);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="AboutLayout" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="350" initialHeight="200">
  <BACKGROUND backgroundColour="ffffffff"/>
  <GENERICCOMPONENT name="logoComonent" id="ec59b0594c99f155" memberName="mLogoComponent"
                    virtualName="" explicitFocusOrder="0" pos="8 8 48 48" class="ImageComponent"
                    params=""/>
  <LABEL name="name label" id="225b7f212e00136a" memberName="mNameLabel"
         virtualName="" explicitFocusOrder="0" pos="56 8 150 24" edTextCol="ff000000"
         edBkgCol="0" labelText="EasySSP v0.1" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="1" italic="0" justification="33"/>
  <LABEL name="short decription" id="a229566877dd7eb5" memberName="mShortDescription"
         virtualName="" explicitFocusOrder="0" pos="8 64 328 48" edTextCol="ff000000"
         edBkgCol="0" labelText="Easy Sound Space Perception is a small and lightweight sound visualization tool."
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <HYPERLINKBUTTON name="home page hyperlink" id="42aaf795d04f4f71" memberName="mHomePageButton"
                   virtualName="" explicitFocusOrder="0" pos="56 32 104 24" tooltip="http://au.tomatl.org/essp"
                   buttonText="au.tomatl.org" connectedEdges="0" needsCallback="0"
                   radioGroupId="0" url="http://au.tomatl.org/essp"/>
  <LABEL name="copyright label" id="3d0118a0d9125ccf" memberName="mCopyrightLabel"
         virtualName="" explicitFocusOrder="0" pos="8 120 328 24" edTextCol="ff000000"
         edBkgCol="0" labelText="(c) 2013-2014 Aleksandr Prokopchuk" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="gpl notice" id="7f4099be698ac09a" memberName="mGplNotice"
         virtualName="" explicitFocusOrder="0" pos="8 160 384 40" edTextCol="ff000000"
         edBkgCol="0" labelText="This program comes with ABSOLUTELY NO WARRANTY;&#10;for details, visit http://www.gnu.org/licenses/gpl.html"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="13.6" bold="0" italic="0" justification="33"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
