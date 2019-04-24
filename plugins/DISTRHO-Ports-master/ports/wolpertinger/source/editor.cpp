/*
  ==============================================================================

  This is an automatically generated file created by the Jucer!

  Creation date:  9 Nov 2010 4:40:41pm

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
#include "synth.h"
#include "about.h"
//[/Headers]

#include "editor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...

struct _myLookAndFeel: public LookAndFeel_V2,
                       public DeletedAtShutdown
{
	_myLookAndFeel()
	{
		setColour(ComboBox::backgroundColourId, Colour(0xFF101010));
		setColour(ComboBox::textColourId, Colour(0xFFF0F0F0));
		setColour(ComboBox::outlineColourId, Colour(0xFF404040));
		setColour(ComboBox::buttonColourId, Colour(0xFF707070));
		setColour(ComboBox::arrowColourId, Colour(0xFF000000));
		setColour(TextButton::buttonColourId, Colour(0xFF405068));
		setColour(TextButton::textColourOnId, Colour(0xFFFFFFFF));
		setColour(TextButton::textColourOffId, Colour(0xFFFFFFFF));
		setColour(PopupMenu::backgroundColourId, Colour(0xFF404040));
		setColour(PopupMenu::textColourId, Colour(0xFFf0f0f0));
		setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xFF000000));

		LookAndFeel::setDefaultLookAndFeel(this);
	}

	~_myLookAndFeel()
	{
	}

	void drawPopupMenuBackground (Graphics& g, int width, int height)
	{
		const Colour background (findColour (PopupMenu::backgroundColourId));
		g.fillAll (background);

		g.setColour (background.overlaidWith (Colour (0x10000000)));
		for (int i = 0; i < height; i += 3)
			g.fillRect (0, i, width, 1);

		g.setColour(Colour(0, 0, 0));
		g.drawRect (0, 0, width, height);
	}

	void drawDocumentWindowTitleBar (DocumentWindow& window,
									 Graphics& g, int w, int h,
									 int titleSpaceX, int titleSpaceW,
									 const Image icon,
									 bool drawTitleTextOnLeft)
	{
		const bool isActive = window.isActiveWindow();

		g.setGradientFill (ColourGradient (window.getBackgroundColour(),
										   0.0f, 0.0f,
										   window.getBackgroundColour().contrasting (isActive ? 0.15f : 0.05f),
										   0.0f, (float) h, false));
		g.fillAll();

		g.setGradientFill (ColourGradient (Colour(0x40FFFFFF),
										   0.0f, 0.0f,
										   window.getBackgroundColour().withAlpha((uint8)0),
										   0.0f, (float)4, false));
		g.fillRect(0,0, w,4);

		g.setGradientFill (ColourGradient (window.getBackgroundColour().withAlpha((uint8)0),
										   0.0f, h-4,
										   Colour(0x80000000),
										   0.0f, (float)h, false));
		g.fillRect(0,h-4, w,4);


		Font font (h * 0.65f, Font::bold);
		g.setFont (font);

		int textW = font.getStringWidth (window.getName());
		int iconW = 0;
		int iconH = 0;

		if (icon.isValid())
		{
			iconH = (int) font.getHeight();
			iconW = icon.getWidth() * iconH / icon.getHeight() + 4;
		}

		textW = jmin (titleSpaceW, textW + iconW);
		int textX = drawTitleTextOnLeft ? titleSpaceX
										: jmax (titleSpaceX, (w - textW) / 2);

		if (textX + textW > titleSpaceX + titleSpaceW)
			textX = titleSpaceX + titleSpaceW - textW;

		if (icon.isValid())
		{
			g.setOpacity (isActive ? 1.0f : 0.6f);
			g.drawImageWithin (icon, textX, (h - iconH) / 2, iconW, iconH,
							   RectanglePlacement::centred, false);
			textX += iconW;
			textW -= iconW;
		}

		if (window.isColourSpecified (DocumentWindow::textColourId) || isColourSpecified (DocumentWindow::textColourId))
			g.setColour (findColour (DocumentWindow::textColourId));
		else
			g.setColour (window.getBackgroundColour().contrasting (isActive ? 0.7f : 0.4f));

		g.drawText (window.getName(), textX, 0, textW, h, Justification::centredLeft, true);
	}
} *myLookAndFeel;

//[/MiscUserDefs]

//==============================================================================
editor::editor (AudioProcessor *const ownerFilter)
    : AudioProcessorEditor(ownerFilter),
      groupComponent2 (0),
      groupComponent (0),
      label (0),
      label2 (0),
      slgain (0),
      slclip (0),
      slsaw (0),
      label3 (0),
      slrect (0),
      label4 (0),
      sltri (0),
      label5 (0),
      sltune (0),
      label6 (0),
      groupComponent3 (0),
      label7 (0),
      slcutoff (0),
      label8 (0),
      slreso (0),
      label9 (0),
      slbandwidth (0),
      label10 (0),
      slpasses (0),
      label11 (0),
      slvelocity (0),
      label12 (0),
      slinertia (0),
      label13 (0),
      slcurcutoff (0),
      label14 (0),
      label15 (0),
      label19 (0),
      label20 (0),
      label16 (0),
      label17 (0),
      label18 (0),
      slattack5 (0),
      slattack6 (0),
      label21 (0),
      label22 (0),
      slattack7 (0),
      knobAttack (0),
      knobDecay (0),
      knobSustain (0),
      knobRelease (0),
      slfilterlimits (0),
      label23 (0),
      oversmplComboBox (0),
      label25 (0),
      cachedImage_scratches_png (0)
{
    addAndMakeVisible (groupComponent2 = new GroupComponent (T("new group"),
                                                             T("Oscillators")));
    groupComponent2->setExplicitFocusOrder (1);
    groupComponent2->setTextLabelPosition (Justification::centredLeft);
    groupComponent2->setColour (GroupComponent::outlineColourId, Colour (0x66ffffff));
    groupComponent2->setColour (GroupComponent::textColourId, Colours::white);

    addAndMakeVisible (groupComponent = new GroupComponent (T("new group"),
                                                            T("Output")));
    groupComponent->setExplicitFocusOrder (1);
    groupComponent->setTextLabelPosition (Justification::centredLeft);
    groupComponent->setColour (GroupComponent::outlineColourId, Colour (0x66ffffff));
    groupComponent->setColour (GroupComponent::textColourId, Colours::white);

    addAndMakeVisible (label = new Label (T("new label"),
                                          T("Gain")));
    label->setFont (Font (15.0000f, Font::plain));
    label->setJustificationType (Justification::centredRight);
    label->setEditable (false, false, false);
    label->setColour (Label::textColourId, Colours::white);
    label->setColour (TextEditor::textColourId, Colours::black);
    label->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label2 = new Label (T("new label"),
                                           T("Clip")));
    label2->setFont (Font (15.0000f, Font::plain));
    label2->setJustificationType (Justification::centredRight);
    label2->setEditable (false, false, false);
    label2->setColour (Label::textColourId, Colours::white);
    label2->setColour (TextEditor::textColourId, Colours::black);
    label2->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slgain = new Slider (T("new slider")));
    slgain->setRange (0, 4, 0.0001);
    slgain->setSliderStyle (Slider::LinearHorizontal);
    slgain->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slgain->setColour (Slider::backgroundColourId, Colour (0x868686));
    slgain->setColour (Slider::textBoxTextColourId, Colours::white);
    slgain->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slgain->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slgain->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slgain->addListener (this);
    slgain->setSkewFactor (0.3);

    addAndMakeVisible (slclip = new Slider (T("new slider")));
    slclip->setRange (0, 5, 0.0001);
    slclip->setSliderStyle (Slider::LinearHorizontal);
    slclip->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slclip->setColour (Slider::backgroundColourId, Colour (0x868686));
    slclip->setColour (Slider::textBoxTextColourId, Colours::white);
    slclip->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slclip->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slclip->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slclip->addListener (this);
    slclip->setSkewFactor (0.5);

    addAndMakeVisible (slsaw = new Slider (T("new slider")));
    slsaw->setRange (0, 1, 0);
    slsaw->setSliderStyle (Slider::Rotary);
    slsaw->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    slsaw->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    slsaw->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    slsaw->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    slsaw->addListener (this);

    addAndMakeVisible (label3 = new Label (T("new label"),
                                           T("Saw")));
    label3->setFont (Font (15.0000f, Font::plain));
    label3->setJustificationType (Justification::centred);
    label3->setEditable (false, false, false);
    label3->setColour (Label::textColourId, Colours::white);
    label3->setColour (TextEditor::textColourId, Colours::black);
    label3->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slrect = new Slider (T("new slider")));
    slrect->setRange (0, 1, 0);
    slrect->setSliderStyle (Slider::Rotary);
    slrect->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    slrect->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    slrect->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    slrect->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    slrect->addListener (this);

    addAndMakeVisible (label4 = new Label (T("new label"),
                                           T("Rect")));
    label4->setFont (Font (15.0000f, Font::plain));
    label4->setJustificationType (Justification::centred);
    label4->setEditable (false, false, false);
    label4->setColour (Label::textColourId, Colours::white);
    label4->setColour (TextEditor::textColourId, Colours::black);
    label4->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (sltri = new Slider (T("new slider")));
    sltri->setRange (0, 1, 0);
    sltri->setSliderStyle (Slider::Rotary);
    sltri->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    sltri->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    sltri->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    sltri->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    sltri->addListener (this);

    addAndMakeVisible (label5 = new Label (T("new label"),
                                           T("Tri")));
    label5->setFont (Font (15.0000f, Font::plain));
    label5->setJustificationType (Justification::centred);
    label5->setEditable (false, false, false);
    label5->setColour (Label::textColourId, Colours::white);
    label5->setColour (TextEditor::textColourId, Colours::black);
    label5->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (sltune = new Slider (T("new slider")));
    sltune->setRange (0, 1, 0);
    sltune->setSliderStyle (Slider::Rotary);
    sltune->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    sltune->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    sltune->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    sltune->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    sltune->addListener (this);

    addAndMakeVisible (label6 = new Label (T("new label"),
                                           T("Tune")));
    label6->setFont (Font (15.0000f, Font::plain));
    label6->setJustificationType (Justification::centred);
    label6->setEditable (false, false, false);
    label6->setColour (Label::textColourId, Colours::white);
    label6->setColour (TextEditor::textColourId, Colours::black);
    label6->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (groupComponent3 = new GroupComponent (T("new group"),
                                                             T("Filter")));
    groupComponent3->setExplicitFocusOrder (1);
    groupComponent3->setTextLabelPosition (Justification::centredLeft);
    groupComponent3->setColour (GroupComponent::outlineColourId, Colour (0x66ffffff));
    groupComponent3->setColour (GroupComponent::textColourId, Colours::white);

    addAndMakeVisible (label7 = new Label (T("new label"),
                                           T("Filter X")));
    label7->setFont (Font (15.0000f, Font::plain));
    label7->setJustificationType (Justification::centredRight);
    label7->setEditable (false, false, false);
    label7->setColour (Label::textColourId, Colours::white);
    label7->setColour (TextEditor::textColourId, Colours::black);
    label7->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slcutoff = new Slider (T("new slider")));
    slcutoff->setRange (0, 4, 0.0001);
    slcutoff->setSliderStyle (Slider::LinearHorizontal);
    slcutoff->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slcutoff->setColour (Slider::backgroundColourId, Colour (0x868686));
    slcutoff->setColour (Slider::textBoxTextColourId, Colours::white);
    slcutoff->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slcutoff->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slcutoff->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slcutoff->addListener (this);
    slcutoff->setSkewFactor (0.5);

    addAndMakeVisible (label8 = new Label (T("new label"),
                                           T("Resonance")));
    label8->setFont (Font (15.0000f, Font::plain));
    label8->setJustificationType (Justification::centredRight);
    label8->setEditable (false, false, false);
    label8->setColour (Label::textColourId, Colours::white);
    label8->setColour (TextEditor::textColourId, Colours::black);
    label8->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slreso = new Slider (T("new slider")));
    slreso->setRange (0, 4, 0.0001);
    slreso->setSliderStyle (Slider::LinearHorizontal);
    slreso->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slreso->setColour (Slider::backgroundColourId, Colour (0x868686));
    slreso->setColour (Slider::textBoxTextColourId, Colours::white);
    slreso->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slreso->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slreso->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slreso->addListener (this);
    slreso->setSkewFactor (0.5);

    addAndMakeVisible (label9 = new Label (T("new label"),
                                           T("Bandwidth")));
    label9->setFont (Font (15.0000f, Font::plain));
    label9->setJustificationType (Justification::centredRight);
    label9->setEditable (false, false, false);
    label9->setColour (Label::textColourId, Colours::white);
    label9->setColour (TextEditor::textColourId, Colours::black);
    label9->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slbandwidth = new Slider (T("new slider")));
    slbandwidth->setRange (0, 4, 0.0001);
    slbandwidth->setSliderStyle (Slider::LinearHorizontal);
    slbandwidth->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slbandwidth->setColour (Slider::backgroundColourId, Colour (0x868686));
    slbandwidth->setColour (Slider::textBoxTextColourId, Colours::white);
    slbandwidth->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slbandwidth->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slbandwidth->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slbandwidth->addListener (this);
    slbandwidth->setSkewFactor (0.5);

    addAndMakeVisible (label10 = new Label (T("new label"),
                                            T("Passes")));
    label10->setFont (Font (15.0000f, Font::plain));
    label10->setJustificationType (Justification::centredRight);
    label10->setEditable (false, false, false);
    label10->setColour (Label::textColourId, Colours::white);
    label10->setColour (TextEditor::textColourId, Colours::black);
    label10->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slpasses = new Slider (T("new slider")));
    slpasses->setRange (0, 4, 0.0001);
    slpasses->setSliderStyle (Slider::LinearHorizontal);
    slpasses->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slpasses->setColour (Slider::backgroundColourId, Colour (0x868686));
    slpasses->setColour (Slider::textBoxTextColourId, Colours::white);
    slpasses->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slpasses->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slpasses->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slpasses->addListener (this);

    addAndMakeVisible (label11 = new Label (T("new label"),
                                            T("Velocity")));
    label11->setFont (Font (15.0000f, Font::plain));
    label11->setJustificationType (Justification::centredRight);
    label11->setEditable (false, false, false);
    label11->setColour (Label::textColourId, Colours::white);
    label11->setColour (TextEditor::textColourId, Colours::black);
    label11->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slvelocity = new Slider (T("new slider")));
    slvelocity->setRange (0, 4, 0.0001);
    slvelocity->setSliderStyle (Slider::LinearHorizontal);
    slvelocity->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slvelocity->setColour (Slider::backgroundColourId, Colour (0x868686));
    slvelocity->setColour (Slider::textBoxTextColourId, Colours::white);
    slvelocity->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slvelocity->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slvelocity->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slvelocity->addListener (this);
    slvelocity->setSkewFactor (0.5);

    addAndMakeVisible (label12 = new Label (T("new label"),
                                            T("Inertia")));
    label12->setFont (Font (15.0000f, Font::plain));
    label12->setJustificationType (Justification::centredRight);
    label12->setEditable (false, false, false);
    label12->setColour (Label::textColourId, Colours::white);
    label12->setColour (TextEditor::textColourId, Colours::black);
    label12->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slinertia = new Slider (T("new slider")));
    slinertia->setRange (0, 4, 0.0001);
    slinertia->setSliderStyle (Slider::LinearHorizontal);
    slinertia->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slinertia->setColour (Slider::backgroundColourId, Colour (0x868686));
    slinertia->setColour (Slider::textBoxTextColourId, Colours::white);
    slinertia->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slinertia->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slinertia->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slinertia->addListener (this);
    slinertia->setSkewFactor (0.5);

    addAndMakeVisible (label13 = new Label (T("new label"),
                                            T("Freq")));
    label13->setFont (Font (15.0000f, Font::plain));
    label13->setJustificationType (Justification::centredRight);
    label13->setEditable (false, false, false);
    label13->setColour (Label::textColourId, Colours::white);
    label13->setColour (TextEditor::textColourId, Colours::black);
    label13->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slcurcutoff = new Slider (T("new slider")));
    slcurcutoff->setRange (0, 4, 0.0001);
    slcurcutoff->setSliderStyle (Slider::LinearHorizontal);
    slcurcutoff->setTextBoxStyle (Slider::TextBoxRight, false, 60, 20);
    slcurcutoff->setColour (Slider::backgroundColourId, Colour (0x868686));
    slcurcutoff->setColour (Slider::textBoxTextColourId, Colours::white);
    slcurcutoff->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slcurcutoff->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slcurcutoff->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slcurcutoff->addListener (this);
    slcurcutoff->setSkewFactor (0.4);

    addAndMakeVisible (label14 = new Label (T("new label"),
                                            T("Waveform")));
    label14->setFont (Font (15.0000f, Font::plain));
    label14->setJustificationType (Justification::centredLeft);
    label14->setEditable (false, false, false);
    label14->setColour (Label::textColourId, Colours::white);
    label14->setColour (TextEditor::textColourId, Colours::black);
    label14->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label15 = new Label (T("new label"),
                                            T("A")));
    label15->setFont (Font (15.0000f, Font::plain));
    label15->setJustificationType (Justification::centred);
    label15->setEditable (false, false, false);
    label15->setColour (Label::textColourId, Colours::white);
    label15->setColour (TextEditor::textColourId, Colours::black);
    label15->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label19 = new Label (T("new label"),
                                            T("Inertia")));
    label19->setFont (Font (15.0000f, Font::plain));
    label19->setJustificationType (Justification::centred);
    label19->setEditable (false, false, false);
    label19->setColour (Label::textColourId, Colours::white);
    label19->setColour (TextEditor::textColourId, Colours::black);
    label19->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label20 = new Label (T("new label"),
                                            T("Velocity")));
    label20->setFont (Font (15.0000f, Font::plain));
    label20->setJustificationType (Justification::centred);
    label20->setEditable (false, false, false);
    label20->setColour (Label::textColourId, Colours::white);
    label20->setColour (TextEditor::textColourId, Colours::black);
    label20->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label16 = new Label (T("new label"),
                                            T("D")));
    label16->setFont (Font (15.0000f, Font::plain));
    label16->setJustificationType (Justification::centred);
    label16->setEditable (false, false, false);
    label16->setColour (Label::textColourId, Colours::white);
    label16->setColour (TextEditor::textColourId, Colours::black);
    label16->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label17 = new Label (T("new label"),
                                            T("S")));
    label17->setFont (Font (15.0000f, Font::plain));
    label17->setJustificationType (Justification::centred);
    label17->setEditable (false, false, false);
    label17->setColour (Label::textColourId, Colours::white);
    label17->setColour (TextEditor::textColourId, Colours::black);
    label17->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label18 = new Label (T("new label"),
                                            T("R")));
    label18->setFont (Font (15.0000f, Font::plain));
    label18->setJustificationType (Justification::centred);
    label18->setEditable (false, false, false);
    label18->setColour (Label::textColourId, Colours::white);
    label18->setColour (TextEditor::textColourId, Colours::black);
    label18->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slattack5 = new Slider (T("new slider")));
    slattack5->setRange (0, 1, 0);
    slattack5->setSliderStyle (Slider::Rotary);
    slattack5->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    slattack5->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    slattack5->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    slattack5->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    slattack5->addListener (this);

    addAndMakeVisible (slattack6 = new Slider (T("new slider")));
    slattack6->setRange (0, 1, 0);
    slattack6->setSliderStyle (Slider::Rotary);
    slattack6->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    slattack6->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    slattack6->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    slattack6->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    slattack6->addListener (this);

    addAndMakeVisible (label21 = new Label (T("new label"),
                                            T("Scale")));
    label21->setFont (Font (15.0000f, Font::plain));
    label21->setJustificationType (Justification::centred);
    label21->setEditable (false, false, false);
    label21->setColour (Label::textColourId, Colours::white);
    label21->setColour (TextEditor::textColourId, Colours::black);
    label21->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (label22 = new Label (T("new label"),
                                            T("Envelope")));
    label22->setFont (Font (15.0000f, Font::plain));
    label22->setJustificationType (Justification::centredLeft);
    label22->setEditable (false, false, false);
    label22->setColour (Label::textColourId, Colours::white);
    label22->setColour (TextEditor::textColourId, Colours::black);
    label22->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (slattack7 = new Slider (T("new slider")));
    slattack7->setRange (0, 1, 0);
    slattack7->setSliderStyle (Slider::Rotary);
    slattack7->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    slattack7->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    slattack7->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    slattack7->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    slattack7->addListener (this);

    addAndMakeVisible (knobAttack = new Slider (T("new slider")));
    knobAttack->setRange (0, 1, 0);
    knobAttack->setSliderStyle (Slider::Rotary);
    knobAttack->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    knobAttack->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    knobAttack->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    knobAttack->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    knobAttack->addListener (this);

    addAndMakeVisible (knobDecay = new Slider (T("new slider")));
    knobDecay->setRange (0, 1, 0);
    knobDecay->setSliderStyle (Slider::Rotary);
    knobDecay->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    knobDecay->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    knobDecay->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    knobDecay->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    knobDecay->addListener (this);

    addAndMakeVisible (knobSustain = new Slider (T("new slider")));
    knobSustain->setRange (0, 1, 0);
    knobSustain->setSliderStyle (Slider::Rotary);
    knobSustain->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    knobSustain->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    knobSustain->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    knobSustain->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    knobSustain->addListener (this);

    addAndMakeVisible (knobRelease = new Slider (T("new slider")));
    knobRelease->setRange (0, 1, 0);
    knobRelease->setSliderStyle (Slider::Rotary);
    knobRelease->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    knobRelease->setColour (Slider::thumbColourId, Colour (0xffbbf2ff));
    knobRelease->setColour (Slider::rotarySliderFillColourId, Colour (0x7f00b9ff));
    knobRelease->setColour (Slider::rotarySliderOutlineColourId, Colour (0x80ffffff));
    knobRelease->addListener (this);

    addAndMakeVisible (slfilterlimits = new Slider (T("new slider")));
    slfilterlimits->setRange (0, 4, 0.0001);
    slfilterlimits->setSliderStyle (Slider::TwoValueHorizontal);
    slfilterlimits->setTextBoxStyle (Slider::NoTextBox, false, 60, 20);
    slfilterlimits->setColour (Slider::backgroundColourId, Colour (0x868686));
    slfilterlimits->setColour (Slider::textBoxTextColourId, Colours::white);
    slfilterlimits->setColour (Slider::textBoxBackgroundColourId, Colour (0x565656));
    slfilterlimits->setColour (Slider::textBoxHighlightColourId, Colour (0x409a9aff));
    slfilterlimits->setColour (Slider::textBoxOutlineColourId, Colour (0x0));
    slfilterlimits->addListener (this);
    slfilterlimits->setSkewFactor (0.4);

    addAndMakeVisible (label23 = new Label (T("new label"),
                                            T("Limits")));
    label23->setFont (Font (15.0000f, Font::plain));
    label23->setJustificationType (Justification::centredRight);
    label23->setEditable (false, false, false);
    label23->setColour (Label::textColourId, Colours::white);
    label23->setColour (TextEditor::textColourId, Colours::black);
    label23->setColour (TextEditor::backgroundColourId, Colour (0x0));

    addAndMakeVisible (oversmplComboBox = new ComboBox (T("new combo box")));
    oversmplComboBox->setEditableText (false);
    oversmplComboBox->setJustificationType (Justification::centredLeft);
    oversmplComboBox->setTextWhenNothingSelected (T("None"));
    oversmplComboBox->setTextWhenNoChoicesAvailable (T("(no choices)"));
    oversmplComboBox->addItem (T("None"), 1);
    oversmplComboBox->addItem (T("8x"), 2);
    oversmplComboBox->addItem (T("16x"), 3);
    oversmplComboBox->addListener (this);

    addAndMakeVisible (label25 = new Label (T("new label"),
                                            T("Oversampling:")));
    label25->setFont (Font (15.0000f, Font::plain));
    label25->setJustificationType (Justification::bottomRight);
    label25->setEditable (false, false, false);
    label25->setColour (Label::textColourId, Colours::white);
    label25->setColour (TextEditor::textColourId, Colours::black);
    label25->setColour (TextEditor::backgroundColourId, Colour (0x0));

    cachedImage_scratches_png = ImageCache::getFromMemory (scratches_png, scratches_pngSize);

    //[UserPreSize]
    //[/UserPreSize]

    setSize (400, 444);

    ((wolp*)ownerFilter)->addChangeListener(this);

    if(!myLookAndFeel) myLookAndFeel= new _myLookAndFeel();

    oversmplComboBox->setLookAndFeel(myLookAndFeel);
}

editor::~editor()
{
    ((wolp*)getAudioProcessor())->removeChangeListener(this);

    deleteAndZero (groupComponent2);
    deleteAndZero (groupComponent);
    deleteAndZero (label);
    deleteAndZero (label2);
    deleteAndZero (slgain);
    deleteAndZero (slclip);
    deleteAndZero (slsaw);
    deleteAndZero (label3);
    deleteAndZero (slrect);
    deleteAndZero (label4);
    deleteAndZero (sltri);
    deleteAndZero (label5);
    deleteAndZero (sltune);
    deleteAndZero (label6);
    deleteAndZero (groupComponent3);
    deleteAndZero (label7);
    deleteAndZero (slcutoff);
    deleteAndZero (label8);
    deleteAndZero (slreso);
    deleteAndZero (label9);
    deleteAndZero (slbandwidth);
    deleteAndZero (label10);
    deleteAndZero (slpasses);
    deleteAndZero (label11);
    deleteAndZero (slvelocity);
    deleteAndZero (label12);
    deleteAndZero (slinertia);
    deleteAndZero (label13);
    deleteAndZero (slcurcutoff);
    deleteAndZero (label14);
    deleteAndZero (label15);
    deleteAndZero (label19);
    deleteAndZero (label20);
    deleteAndZero (label16);
    deleteAndZero (label17);
    deleteAndZero (label18);
    deleteAndZero (slattack5);
    deleteAndZero (slattack6);
    deleteAndZero (label21);
    deleteAndZero (label22);
    deleteAndZero (slattack7);
    deleteAndZero (knobAttack);
    deleteAndZero (knobDecay);
    deleteAndZero (knobSustain);
    deleteAndZero (knobRelease);
    deleteAndZero (slfilterlimits);
    deleteAndZero (label23);
    deleteAndZero (oversmplComboBox);
    deleteAndZero (label25);
//    ImageCache::release (cachedImage_scratches_png);

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void editor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colour (0xff272727));

    g.setColour (Colours::black.withAlpha (0.0750f));
    g.drawImageWithin (cachedImage_scratches_png,
                       2, 2, 300, 180,
                       RectanglePlacement::centred,
                       false);

    g.setColour (Colours::black.withAlpha (0.0750f));
    g.drawImageWithin (cachedImage_scratches_png,
                       302, 2, 300, 180,
                       RectanglePlacement::centred,
                       false);

    g.setGradientFill (ColourGradient (Colour (0xff272727),
                                       92.0f, 0.0f,
                                       Colour (0x272727),
                                       92.0f, 8.0f,
                                       false));
    g.fillRect (0, 0, 408, 16);

    g.setColour (Colours::black.withAlpha (0.0750f));
    g.drawImageWithin (cachedImage_scratches_png,
                       2, 182, 300, 180,
                       RectanglePlacement::centred,
                       false);

    g.setColour (Colours::black.withAlpha (0.0750f));
    g.drawImageWithin (cachedImage_scratches_png,
                       302, 182, 300, 180,
                       RectanglePlacement::centred,
                       false);

    g.setColour (Colours::black.withAlpha (0.0750f));
    g.drawImageWithin (cachedImage_scratches_png,
                       2, 362, 300, 180,
                       RectanglePlacement::centred,
                       false);

    g.setColour (Colours::black.withAlpha (0.0750f));
    g.drawImageWithin (cachedImage_scratches_png,
                       302, 362, 300, 180,
                       RectanglePlacement::centred,
                       false);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void editor::resized()
{
    groupComponent2->setBounds (8, 8, 384, 140);
    groupComponent->setBounds (8, 393, 384, 80);
    label->setBounds (16, 417, 56, 16);
    label2->setBounds (16, 441, 56, 16);
    slgain->setBounds (80, 417, 304, 16);
    slclip->setBounds (80, 441, 304, 16);
    slsaw->setBounds (24, 48, 40, 40);
    label3->setBounds (24, 88, 40, 16);
    slrect->setBounds (72, 48, 40, 40);
    label4->setBounds (72, 88, 40, 16);
    sltri->setBounds (120, 48, 40, 40);
    label5->setBounds (120, 88, 40, 16);
    sltune->setBounds (168, 48, 40, 40);
    label6->setBounds (168, 88, 40, 16);
    groupComponent3->setBounds (8, 157, 384, 227);
    label7->setBounds (16, 184, 64, 16);
    slcutoff->setBounds (80, 184, 304, 16);
    label8->setBounds (16, 209, 64, 15);
    slreso->setBounds (80, 209, 304, 15);
    label9->setBounds (16, 233, 64, 15);
    slbandwidth->setBounds (80, 233, 304, 15);
    label10->setBounds (16, 257, 64, 15);
    slpasses->setBounds (80, 257, 304, 15);
    label11->setBounds (16, 281, 64, 15);
    slvelocity->setBounds (80, 281, 304, 15);
    label12->setBounds (16, 304, 64, 16);
    slinertia->setBounds (80, 304, 304, 16);
    label13->setBounds (16, 328, 64, 15);
    slcurcutoff->setBounds (80, 328, 304, 16);
    label14->setBounds (24, (48) + (40) / 2 + -28 - ((16) / 2), 80, 16);
    label15->setBounds (226, 89, 32, 16);
    label19->setBounds (720, 168, 48, 16);
    label20->setBounds (664, 168, 56, 16);
    label16->setBounds (266, 89, 32, 16);
    label17->setBounds (306, 89, 32, 16);
    label18->setBounds (346, 89, 32, 16);
    slattack5->setBounds (672, 136, 40, 32);
    slattack6->setBounds (728, 136, 32, 32);
    label21->setBounds (616, 168, 48, 16);
    label22->setBounds (224, 32, 136, 16);
    slattack7->setBounds (624, 136, 32, 32);
    knobAttack->setBounds (226, 48, 32, 41);
    knobDecay->setBounds (266, 48, 32, 41);
    knobSustain->setBounds (306, 48, 32, 41);
    knobRelease->setBounds (346, 48, 32, 41);
    slfilterlimits->setBounds (80, 352, 244, 16);
    label23->setBounds (16, 352, 64, 16);
    oversmplComboBox->setBounds (128, 116, 56, 18);
    label25->setBounds (22, 116, 102, 18);
    //[UserResized] Add your own custom resize handling here..
#if 1
    #define initslider(slidername, param)\
    slidername->setRange(synth->paraminfos[wolp::param].min, synth->paraminfos[wolp::param].max);	\
    slidername->setValue(synth->getParameter(wolp::param)*(synth->paraminfos[wolp::param].max-synth->paraminfos[wolp::param].min),dontSendNotification);
    wolp *synth= (wolp*)getAudioProcessor();
    initslider(slsaw, gsaw);
    initslider(slrect, grect);
    initslider(sltri, gtri);
    initslider(sltune, tune);
    initslider(slgain, gain);
    initslider(slclip, clip);

    initslider(slcutoff, cutoff);
    initslider(slreso, resonance);
    initslider(slbandwidth, bandwidth);
    initslider(slpasses, nfilters);
    initslider(slvelocity, velocity);
    initslider(slinertia, inertia);
    initslider(slcurcutoff, curcutoff);

    initslider(knobAttack, attack);
    initslider(knobDecay, decay);
    initslider(knobSustain, sustain);
    initslider(knobRelease, release);

    slpasses->setRange(synth->paraminfos[wolp::nfilters].min, synth->paraminfos[wolp::nfilters].max, 1.0);
    slpasses->setValue(synth->getParameter(wolp::nfilters)*(synth->paraminfos[wolp::nfilters].max-synth->paraminfos[wolp::nfilters].min), dontSendNotification);

    slfilterlimits->setRange(synth->paraminfos[wolp::filtermin].min, synth->paraminfos[wolp::filtermin].max);
    slfilterlimits->setMaxValue(synth->getParameter(wolp::filtermax)*(synth->paraminfos[wolp::filtermax].max-synth->paraminfos[wolp::filtermax].min), dontSendNotification);
    slfilterlimits->setMinValue(synth->getParameter(wolp::filtermin)*(synth->paraminfos[wolp::filtermin].max-synth->paraminfos[wolp::filtermin].min), dontSendNotification);
    #undef initslider
#endif
    //[/UserResized]
}

void editor::sliderValueChanged (Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
#if 1
    wolp *synth= (wolp*)getAudioProcessor();
    #define updateval(param)\
    synth->setParameterNotifyingHost( wolp::param, sliderThatWasMoved->getValue() / \
		(synth->paraminfos[wolp::param].max-synth->paraminfos[wolp::param].min) )
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == slgain)
    {
        //[UserSliderCode_slgain] -- add your slider handling code here..
		updateval(gain);
        //[/UserSliderCode_slgain]
    }
    else if (sliderThatWasMoved == slclip)
    {
        //[UserSliderCode_slclip] -- add your slider handling code here..
		updateval(clip);
        //[/UserSliderCode_slclip]
    }
    else if (sliderThatWasMoved == slsaw)
    {
        //[UserSliderCode_slsaw] -- add your slider handling code here..
		updateval(gsaw);
        //[/UserSliderCode_slsaw]
    }
    else if (sliderThatWasMoved == slrect)
    {
        //[UserSliderCode_slrect] -- add your slider handling code here..
		updateval(grect);
        //[/UserSliderCode_slrect]
    }
    else if (sliderThatWasMoved == sltri)
    {
        //[UserSliderCode_sltri] -- add your slider handling code here..
		updateval(gtri);
        //[/UserSliderCode_sltri]
    }
    else if (sliderThatWasMoved == sltune)
    {
        //[UserSliderCode_sltune] -- add your slider handling code here..
		updateval(tune);
        //[/UserSliderCode_sltune]
    }
    else if (sliderThatWasMoved == slcutoff)
    {
        //[UserSliderCode_slcutoff] -- add your slider handling code here..
		updateval(cutoff);
        //[/UserSliderCode_slcutoff]
    }
    else if (sliderThatWasMoved == slreso)
    {
        //[UserSliderCode_slreso] -- add your slider handling code here..
		updateval(resonance);
        //[/UserSliderCode_slreso]
    }
    else if (sliderThatWasMoved == slbandwidth)
    {
        //[UserSliderCode_slbandwidth] -- add your slider handling code here..
		updateval(bandwidth);
        //[/UserSliderCode_slbandwidth]
    }
    else if (sliderThatWasMoved == slpasses)
    {
        //[UserSliderCode_slpasses] -- add your slider handling code here..
		updateval(nfilters);
        //[/UserSliderCode_slpasses]
    }
    else if (sliderThatWasMoved == slvelocity)
    {
        //[UserSliderCode_slvelocity] -- add your slider handling code here..
		updateval(velocity);
        //[/UserSliderCode_slvelocity]
    }
    else if (sliderThatWasMoved == slinertia)
    {
        //[UserSliderCode_slinertia] -- add your slider handling code here..
		updateval(inertia);
        //[/UserSliderCode_slinertia]
    }
    else if (sliderThatWasMoved == slcurcutoff)
    {
        //[UserSliderCode_slcurcutoff] -- add your slider handling code here..
		updateval(curcutoff);
        //[/UserSliderCode_slcurcutoff]
    }
    else if (sliderThatWasMoved == slattack5)
    {
        //[UserSliderCode_slattack5] -- add your slider handling code here..
        //[/UserSliderCode_slattack5]
    }
    else if (sliderThatWasMoved == slattack6)
    {
        //[UserSliderCode_slattack6] -- add your slider handling code here..
        //[/UserSliderCode_slattack6]
    }
    else if (sliderThatWasMoved == slattack7)
    {
        //[UserSliderCode_slattack7] -- add your slider handling code here..
        //[/UserSliderCode_slattack7]
    }
    else if (sliderThatWasMoved == knobAttack)
    {
        //[UserSliderCode_knobAttack] -- add your slider handling code here..
        updateval(attack);
        //[/UserSliderCode_knobAttack]
    }
    else if (sliderThatWasMoved == knobDecay)
    {
        //[UserSliderCode_knobDecay] -- add your slider handling code here..
        updateval(decay);
        //[/UserSliderCode_knobDecay]
    }
    else if (sliderThatWasMoved == knobSustain)
    {
        //[UserSliderCode_knobSustain] -- add your slider handling code here..
        updateval(sustain);
        //[/UserSliderCode_knobSustain]
    }
    else if (sliderThatWasMoved == knobRelease)
    {
        //[UserSliderCode_knobRelease] -- add your slider handling code here..
        updateval(release);
        //[/UserSliderCode_knobRelease]
    }
    else if (sliderThatWasMoved == slfilterlimits)
    {
        //[UserSliderCode_slfilterlimits] -- add your slider handling code here..
		synth->setParameterNotifyingHost( wolp::filtermin, sliderThatWasMoved->getMinValue() /
			(synth->paraminfos[wolp::filtermin].max-synth->paraminfos[wolp::filtermin].min) );
		synth->setParameterNotifyingHost( wolp::filtermax, sliderThatWasMoved->getMaxValue() /
			(synth->paraminfos[wolp::filtermax].max-synth->paraminfos[wolp::filtermax].min) );
        //[/UserSliderCode_slfilterlimits]
    }

    //[UsersliderValueChanged_Post]
    #undef updateval
#endif
    //[/UsersliderValueChanged_Post]
}

void editor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == oversmplComboBox)
    {
        //[UserComboBoxCode_oversmplComboBox] -- add your combo box handling code here..
        int idx= oversmplComboBox->getSelectedItemIndex();
        wolp *synth= (wolp*)getAudioProcessor();
        synth->setParameterNotifyingHost(wolp::oversampling, double(idx)/2.0);
        //[/UserComboBoxCode_oversmplComboBox]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void editor::changeListenerCallback(ChangeBroadcaster *objectThatHasChanged)
{
//    intptr_t idx= (intptr_t)objectThatHasChanged;
	wolp *synth= (wolp*)getAudioProcessor();
#define updateslider(name, slidername)		\
		case wolp::name:	\
			slidername->setValue( synth->params[wolp::name] * \
								  (synth->paraminfos[idx].max-synth->paraminfos[idx].min), dontSendNotification); \
			break;

    for(int idx= 0; idx<wolp::param_size; idx++)
        if(synth->paraminfos[idx].dirty)
        {
            synth->paraminfos[idx].dirty= false;

            switch(idx)
            {
                updateslider (gain, slgain);
                updateslider (clip, slclip);
                updateslider (gsaw, slsaw);
                updateslider (grect, slrect);
                updateslider (gtri, sltri);
                updateslider (tune, sltune);
                updateslider (cutoff, slcutoff);
                updateslider (resonance, slreso);
                updateslider (bandwidth, slbandwidth);
                updateslider (velocity, slvelocity);
                updateslider (inertia, slinertia);
                updateslider (nfilters, slpasses);
                updateslider (attack, knobAttack);
                updateslider (decay, knobDecay);
                updateslider (sustain, knobSustain);
                updateslider (release, knobRelease);
                case wolp::curcutoff:
                    slcurcutoff->setValue( synth->params[wolp::curcutoff] *
                                          (synth->paraminfos[idx].max-synth->paraminfos[idx].min), dontSendNotification );
                    break;
                case wolp::filtermin:
                    slfilterlimits->setMinValue( synth->params[wolp::filtermin] *
                                (synth->paraminfos[idx].max-synth->paraminfos[idx].min), dontSendNotification );
                    break;
                case wolp::filtermax:
                    slfilterlimits->setMaxValue( synth->params[wolp::filtermax] *
                                (synth->paraminfos[idx].max-synth->paraminfos[idx].min), dontSendNotification );
                    break;

                case wolp::oversampling:
                {
                    int idx= int(synth->getParameter(wolp::oversampling)*2);
                    oversmplComboBox->setSelectedItemIndex( idx, sendNotification );
                    break;
                }

                default:
                    printf("Unknown Object Changed: %i\n", idx);
                    break;
            }
        }
#undef updateslider
}

//==============================================================================
#if 0
/*  -- Jucer information section --

    This is where the Jucer puts all of its metadata, so don't change anything in here!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="editor" componentName=""
                 parentClasses="public AudioProcessorEditor, public ChangeListener"
                 constructorParams="AudioProcessor *const ownerFilter" variableInitialisers="AudioProcessorEditor(ownerFilter)"
                 snapPixels="4" snapActive="1" snapShown="0" overlayOpacity="0.330000013"
                 fixedSize="0" initialWidth="400" initialHeight="444">
  <BACKGROUND backgroundColour="ff272727">
    <IMAGE pos="2 2 300 180" resource="scratches_png" opacity="0.075" mode="1"/>
    <IMAGE pos="302 2 300 180" resource="scratches_png" opacity="0.075"
           mode="1"/>
    <RECT pos="0 0 408 16" fill="linear: 92 0, 92 8, 0=ff272727, 1=272727"
          hasStroke="0"/>
    <IMAGE pos="2 182 300 180" resource="scratches_png" opacity="0.075"
           mode="1"/>
    <IMAGE pos="302 182 300 180" resource="scratches_png" opacity="0.075"
           mode="1"/>
    <IMAGE pos="2 362 300 180" resource="scratches_png" opacity="0.075"
           mode="1"/>
    <IMAGE pos="302 362 300 180" resource="scratches_png" opacity="0.075"
           mode="1"/>
  </BACKGROUND>
  <GROUPCOMPONENT name="new group" id="e5e178cd242b6420" memberName="groupComponent2"
                  virtualName="" explicitFocusOrder="1" pos="8 8 384 140" outlinecol="66ffffff"
                  textcol="ffffffff" title="Oscillators" textpos="33"/>
  <GROUPCOMPONENT name="new group" id="ad99a16746beb177" memberName="groupComponent"
                  virtualName="" explicitFocusOrder="1" pos="8 393 384 80" outlinecol="66ffffff"
                  textcol="ffffffff" title="Output" textpos="33"/>
  <LABEL name="new label" id="49509c28b7b7d12d" memberName="label" virtualName=""
         explicitFocusOrder="0" pos="16 417 56 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Gain" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <LABEL name="new label" id="155d710d9c4a7bd0" memberName="label2" virtualName=""
         explicitFocusOrder="0" pos="16 441 56 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Clip" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="8a7b903812862b6c" memberName="slgain" virtualName=""
          explicitFocusOrder="0" pos="80 417 304 16" bkgcol="868686" textboxtext="ffffffff"
          textboxbkgd="565656" textboxhighlight="409a9aff" textboxoutline="0"
          min="0" max="4" int="0.0001" style="LinearHorizontal" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="0.3"/>
  <SLIDER name="new slider" id="5f153060103ce45b" memberName="slclip" virtualName=""
          explicitFocusOrder="0" pos="80 441 304 16" bkgcol="868686" textboxtext="ffffffff"
          textboxbkgd="565656" textboxhighlight="409a9aff" textboxoutline="0"
          min="0" max="5" int="0.0001" style="LinearHorizontal" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="0.5"/>
  <SLIDER name="new slider" id="9ed6f0186f5ff937" memberName="slsaw" virtualName=""
          explicitFocusOrder="0" pos="24 48 40 40" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="eb3d9855455c7a5c" memberName="label3" virtualName=""
         explicitFocusOrder="0" pos="24 88 40 16" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Saw" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="36"/>
  <SLIDER name="new slider" id="f984b6ce867a512c" memberName="slrect" virtualName=""
          explicitFocusOrder="0" pos="72 48 40 40" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="4df8bd0f758110b0" memberName="label4" virtualName=""
         explicitFocusOrder="0" pos="72 88 40 16" textCol="ffffffff" edTextCol="ff000000"
         edBkgCol="0" labelText="Rect" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15"
         bold="0" italic="0" justification="36"/>
  <SLIDER name="new slider" id="787a4f0997781dd1" memberName="sltri" virtualName=""
          explicitFocusOrder="0" pos="120 48 40 40" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="881dba415f31e105" memberName="label5" virtualName=""
         explicitFocusOrder="0" pos="120 88 40 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Tri" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <SLIDER name="new slider" id="34b4e27fcd2e1063" memberName="sltune" virtualName=""
          explicitFocusOrder="0" pos="168 48 40 40" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="2466156e2db40baf" memberName="label6" virtualName=""
         explicitFocusOrder="0" pos="168 88 40 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Tune" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <GROUPCOMPONENT name="new group" id="7ce402f8c62d5932" memberName="groupComponent3"
                  virtualName="" explicitFocusOrder="1" pos="8 157 384 227" outlinecol="66ffffff"
                  textcol="ffffffff" title="Filter" textpos="33"/>
  <LABEL name="new label" id="e57e597377fef365" memberName="label7" virtualName=""
         explicitFocusOrder="0" pos="16 184 64 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Filter X" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="df34f7f508b7adb0" memberName="slcutoff"
          virtualName="" explicitFocusOrder="0" pos="80 184 304 16" bkgcol="868686"
          textboxtext="ffffffff" textboxbkgd="565656" textboxhighlight="409a9aff"
          textboxoutline="0" min="0" max="4" int="0.0001" style="LinearHorizontal"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="new label" id="e2f509e8c9131813" memberName="label8" virtualName=""
         explicitFocusOrder="0" pos="16 209 64 15" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Resonance" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="35786a165f16578e" memberName="slreso" virtualName=""
          explicitFocusOrder="0" pos="80 209 304 15" bkgcol="868686" textboxtext="ffffffff"
          textboxbkgd="565656" textboxhighlight="409a9aff" textboxoutline="0"
          min="0" max="4" int="0.0001" style="LinearHorizontal" textBoxPos="TextBoxRight"
          textBoxEditable="1" textBoxWidth="60" textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="new label" id="cefa7252c084bf3a" memberName="label9" virtualName=""
         explicitFocusOrder="0" pos="16 233 64 15" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Bandwidth" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="5362f75ced0b9ac9" memberName="slbandwidth"
          virtualName="" explicitFocusOrder="0" pos="80 233 304 15" bkgcol="868686"
          textboxtext="ffffffff" textboxbkgd="565656" textboxhighlight="409a9aff"
          textboxoutline="0" min="0" max="4" int="0.0001" style="LinearHorizontal"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="new label" id="adcc1d907bb16758" memberName="label10" virtualName=""
         explicitFocusOrder="0" pos="16 257 64 15" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Passes" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="29e98c8c7765790" memberName="slpasses"
          virtualName="" explicitFocusOrder="0" pos="80 257 304 15" bkgcol="868686"
          textboxtext="ffffffff" textboxbkgd="565656" textboxhighlight="409a9aff"
          textboxoutline="0" min="0" max="4" int="0.0001" style="LinearHorizontal"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="eae3af57dfc37cd5" memberName="label11" virtualName=""
         explicitFocusOrder="0" pos="16 281 64 15" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Velocity" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="f1f951fe4565a7db" memberName="slvelocity"
          virtualName="" explicitFocusOrder="0" pos="80 281 304 15" bkgcol="868686"
          textboxtext="ffffffff" textboxbkgd="565656" textboxhighlight="409a9aff"
          textboxoutline="0" min="0" max="4" int="0.0001" style="LinearHorizontal"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="new label" id="145c7f21c5783bd3" memberName="label12" virtualName=""
         explicitFocusOrder="0" pos="16 304 64 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Inertia" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="1f326d02c58973a" memberName="slinertia"
          virtualName="" explicitFocusOrder="0" pos="80 304 304 16" bkgcol="868686"
          textboxtext="ffffffff" textboxbkgd="565656" textboxhighlight="409a9aff"
          textboxoutline="0" min="0" max="4" int="0.0001" style="LinearHorizontal"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="0.5"/>
  <LABEL name="new label" id="77aa4e9b34a8fa81" memberName="label13" virtualName=""
         explicitFocusOrder="0" pos="16 328 64 15" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Freq" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <SLIDER name="new slider" id="88030f6cab98eb34" memberName="slcurcutoff"
          virtualName="" explicitFocusOrder="0" pos="80 328 304 16" bkgcol="868686"
          textboxtext="ffffffff" textboxbkgd="565656" textboxhighlight="409a9aff"
          textboxoutline="0" min="0" max="4" int="0.0001" style="LinearHorizontal"
          textBoxPos="TextBoxRight" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="0.4"/>
  <LABEL name="new label" id="bf2b92c62f94cc75" memberName="label14" virtualName=""
         explicitFocusOrder="0" pos="24 -28Cc 80 16" posRelativeY="9ed6f0186f5ff937"
         textCol="ffffffff" edTextCol="ff000000" edBkgCol="0" labelText="Waveform"
         editableSingleClick="0" editableDoubleClick="0" focusDiscardsChanges="0"
         fontname="Default font" fontsize="15" bold="0" italic="0" justification="33"/>
  <LABEL name="new label" id="788f76f9610aa084" memberName="label15" virtualName=""
         explicitFocusOrder="0" pos="226 89 32 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="A" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="c5f8428d8d54eb65" memberName="label19" virtualName=""
         explicitFocusOrder="0" pos="720 168 48 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Inertia" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="e21ba2fabc5b6c77" memberName="label20" virtualName=""
         explicitFocusOrder="0" pos="664 168 56 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Velocity" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="d263507c08f0e588" memberName="label16" virtualName=""
         explicitFocusOrder="0" pos="266 89 32 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="D" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="1a960ef97d83f53a" memberName="label17" virtualName=""
         explicitFocusOrder="0" pos="306 89 32 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="S" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="8a53fb5fd12f582b" memberName="label18" virtualName=""
         explicitFocusOrder="0" pos="346 89 32 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="R" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <SLIDER name="new slider" id="e34f9594ade717e" memberName="slattack5"
          virtualName="" explicitFocusOrder="0" pos="672 136 40 32" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="fccef4f71bc5a237" memberName="slattack6"
          virtualName="" explicitFocusOrder="0" pos="728 136 32 32" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <LABEL name="new label" id="9c19403fd33058bb" memberName="label21" virtualName=""
         explicitFocusOrder="0" pos="616 168 48 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Scale" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="94d601be4e7bf89b" memberName="label22" virtualName=""
         explicitFocusOrder="0" pos="224 32 136 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Envelope" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="33"/>
  <SLIDER name="new slider" id="9c6a0f0cb6d7c451" memberName="slattack7"
          virtualName="" explicitFocusOrder="0" pos="624 136 32 32" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="9b7cc6b66815757b" memberName="knobAttack"
          virtualName="" explicitFocusOrder="0" pos="226 48 32 41" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="dbcf879a8e4f456b" memberName="knobDecay"
          virtualName="" explicitFocusOrder="0" pos="266 48 32 41" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="ba560858e5b507f3" memberName="knobSustain"
          virtualName="" explicitFocusOrder="0" pos="306 48 32 41" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="89e012ddb565692c" memberName="knobRelease"
          virtualName="" explicitFocusOrder="0" pos="346 48 32 41" thumbcol="ffbbf2ff"
          rotarysliderfill="7f00b9ff" rotaryslideroutline="80ffffff" min="0"
          max="1" int="0" style="Rotary" textBoxPos="NoTextBox" textBoxEditable="1"
          textBoxWidth="80" textBoxHeight="20" skewFactor="1"/>
  <SLIDER name="new slider" id="806ed68c0ceb178a" memberName="slfilterlimits"
          virtualName="" explicitFocusOrder="0" pos="80 352 244 16" bkgcol="868686"
          textboxtext="ffffffff" textboxbkgd="565656" textboxhighlight="409a9aff"
          textboxoutline="0" min="0" max="4" int="0.0001" style="TwoValueHorizontal"
          textBoxPos="NoTextBox" textBoxEditable="1" textBoxWidth="60"
          textBoxHeight="20" skewFactor="0.4"/>
  <LABEL name="new label" id="b883df02b57d8ed8" memberName="label23" virtualName=""
         explicitFocusOrder="0" pos="16 352 64 16" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Limits" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="34"/>
  <COMBOBOX name="new combo box" id="cf1a54db9ee6988d" memberName="oversmplComboBox"
            virtualName="" explicitFocusOrder="0" pos="128 116 56 18" editable="0"
            layout="33" items="None&#10;8x&#10;16x" textWhenNonSelected="None"
            textWhenNoItems="(no choices)"/>
  <LABEL name="new label" id="2bbccba2db676bcc" memberName="label25" virtualName=""
         explicitFocusOrder="0" pos="22 116 102 18" textCol="ffffffff"
         edTextCol="ff000000" edBkgCol="0" labelText="Oversampling:" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="15" bold="0" italic="0" justification="18"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: scratches_png, 23278, "../res/scratches.png"
static const unsigned char resource_editor_scratches_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,1,44,0,0,0,180,8,0,0,0,0,11,84,78,51,0,0,0,1,115,82,71,66,0,174,206,28,233,0,0,0,9,112,
72,89,115,0,0,11,19,0,0,11,19,1,0,154,156,24,0,0,0,7,116,73,77,69,7,218,11,6,16,8,51,70,229,179,74,0,0,32,0,73,68,65,84,120,94,116,188,43,150,36,59,178,168,253,237,172,229,119,45,145,26,65,0,37,80,131,
13,242,64,39,1,140,120,3,107,24,35,40,224,83,208,32,212,172,128,207,34,72,2,35,126,129,81,31,65,1,15,32,82,32,71,224,196,193,15,164,200,172,238,243,95,237,170,218,17,254,144,76,246,126,41,254,250,23,146,
53,140,206,115,24,37,171,56,128,64,86,43,219,189,100,69,112,105,79,244,103,77,77,49,20,3,64,49,197,212,212,244,121,95,50,42,228,130,27,183,187,130,184,169,184,221,14,8,247,219,33,120,24,183,190,182,21,
0,23,112,163,208,96,16,23,71,250,130,244,239,125,217,48,226,66,166,224,72,70,65,128,92,158,240,117,32,12,53,20,36,23,28,113,161,239,192,244,191,103,255,124,71,178,254,215,53,201,138,188,8,220,248,194,
149,112,131,130,139,32,0,106,202,161,155,154,121,71,138,111,242,196,142,154,221,212,64,21,212,12,76,5,202,243,126,232,136,116,172,112,215,96,108,65,101,147,219,104,198,161,119,193,195,225,35,132,54,29,
184,131,35,69,51,69,28,112,113,232,11,3,100,188,61,169,34,35,8,20,216,12,87,5,113,39,3,8,136,1,34,210,159,198,4,245,62,129,67,16,80,115,156,224,193,248,90,67,4,52,100,21,32,16,62,87,118,69,242,139,139,
143,13,211,2,18,92,238,154,93,112,64,112,12,92,228,16,84,32,59,142,28,89,193,12,196,208,187,96,218,167,212,160,193,145,252,36,203,61,11,133,173,88,46,128,221,9,199,61,56,219,184,161,136,148,205,25,17,
119,8,98,210,129,117,112,71,115,227,19,113,65,252,139,204,138,128,128,118,246,201,158,225,14,130,152,129,184,22,119,92,4,5,112,87,67,81,76,221,173,189,106,224,28,25,163,109,67,238,24,13,167,134,185,131,
140,141,82,7,199,115,101,1,241,242,215,191,219,170,157,25,195,232,86,28,187,141,30,70,151,220,120,219,37,83,218,191,206,231,48,212,62,101,15,131,146,161,108,119,62,247,102,74,56,196,37,151,237,126,187,
163,132,35,28,225,192,52,140,78,56,194,184,29,141,129,155,112,8,142,192,147,82,166,214,36,7,227,115,82,123,130,32,185,184,224,226,166,180,157,131,149,172,226,226,242,148,52,186,12,182,55,112,36,107,227,
4,83,211,0,135,193,237,126,59,128,182,21,128,112,47,14,132,163,47,249,212,8,29,89,95,130,43,222,240,131,120,131,65,112,176,210,180,77,121,114,141,193,147,26,77,43,148,92,114,187,165,207,121,172,100,40,
228,178,113,4,142,112,135,219,93,237,118,24,26,14,218,198,159,8,115,201,197,27,10,120,50,54,25,26,125,172,228,210,68,82,200,197,197,27,165,120,42,53,43,13,215,210,116,157,21,112,211,79,56,37,83,200,229,
19,87,13,249,29,25,95,216,164,93,19,252,19,81,208,190,35,224,47,46,208,105,105,34,141,140,142,139,56,18,20,112,4,81,68,27,239,88,99,5,5,45,24,162,106,170,93,51,148,126,167,207,95,40,237,219,113,132,227,
110,168,234,113,11,183,209,10,32,2,55,39,192,1,142,119,193,201,154,49,64,12,156,66,41,77,72,92,59,135,57,234,56,170,82,4,51,111,74,94,59,7,122,211,239,218,113,37,40,168,146,181,128,102,147,173,9,26,170,
152,221,49,16,180,4,62,199,97,65,220,145,227,75,79,26,89,0,119,249,235,95,64,71,242,19,237,55,14,196,197,195,33,208,165,79,220,160,0,157,193,1,163,147,179,179,144,91,105,50,252,164,148,65,105,192,155,
134,195,52,28,178,221,81,163,144,111,71,167,226,118,0,2,141,97,144,172,146,181,177,74,166,180,63,78,131,14,43,141,227,59,143,225,88,155,191,228,66,86,186,92,180,187,13,180,102,99,10,228,198,136,124,14,
83,140,226,95,124,20,14,4,186,225,207,74,23,74,201,165,25,26,249,246,15,169,192,9,136,39,193,229,227,245,188,198,90,137,245,20,95,38,98,214,180,164,88,211,94,242,132,199,231,114,50,164,232,213,134,184,
236,76,107,89,7,211,37,237,131,165,244,118,18,78,96,47,235,180,78,219,41,11,251,43,73,30,212,225,53,25,101,251,253,227,247,181,50,156,132,199,137,196,232,17,159,157,42,203,52,45,67,18,106,170,145,33,45,
101,98,72,75,2,136,107,98,206,137,42,85,92,42,196,60,197,56,65,133,84,83,100,90,146,56,182,79,84,89,82,116,106,133,125,79,105,71,99,94,75,149,184,12,128,84,176,132,12,182,195,186,15,21,174,21,108,255,
251,26,189,214,36,17,34,131,37,222,58,90,150,169,198,138,248,183,127,212,231,230,89,246,201,133,7,226,21,132,184,12,188,253,92,215,178,164,121,89,247,148,22,93,134,240,0,194,41,149,101,159,221,230,193,
134,93,247,85,235,158,246,52,47,236,36,251,177,164,247,4,178,78,195,199,143,252,35,102,116,127,123,223,215,185,114,134,225,239,95,195,251,175,239,252,76,225,128,83,106,248,253,248,24,30,181,49,204,156,
215,82,137,46,113,153,168,164,8,85,166,10,150,234,190,207,12,182,167,74,172,85,98,100,202,195,50,68,26,46,107,244,68,133,121,168,149,165,60,105,154,246,68,218,231,172,41,214,186,40,88,170,88,154,43,213,
110,175,105,94,217,247,36,46,75,154,167,159,223,31,225,68,188,214,8,53,53,22,162,34,195,199,213,17,183,111,255,232,184,34,250,174,149,90,33,86,228,227,226,53,73,189,14,243,58,13,54,204,195,188,164,125,
87,78,194,201,41,110,59,197,77,151,253,118,29,12,181,180,207,235,190,82,86,101,255,254,106,36,88,74,158,47,172,195,130,154,190,107,122,251,185,239,111,220,255,126,149,105,250,249,250,54,220,247,183,225,
172,97,124,132,241,231,92,99,149,232,194,170,21,170,224,115,158,150,185,70,168,209,65,166,74,218,135,106,154,144,232,16,1,6,18,117,33,73,149,42,142,37,164,198,10,50,212,250,220,86,2,102,79,216,58,215,
61,193,92,145,129,101,79,225,245,125,94,190,255,24,118,246,65,88,247,117,90,95,57,155,160,214,200,231,4,136,115,205,137,24,135,111,255,51,156,182,39,194,89,101,192,222,78,16,39,60,206,24,99,116,98,244,
84,153,151,193,166,193,202,0,112,34,181,154,206,67,149,117,71,175,249,237,87,137,235,206,10,186,79,131,165,244,158,118,197,210,219,99,206,147,239,59,202,158,230,229,109,28,222,126,253,125,231,239,171,
127,60,222,6,248,251,239,227,36,28,53,240,251,53,214,42,30,163,215,185,2,68,34,83,46,68,112,161,74,173,181,237,59,129,13,213,146,0,30,99,5,118,82,165,74,78,198,158,98,173,72,223,169,84,0,219,19,203,190,
167,93,151,180,239,73,242,254,246,243,237,58,13,114,89,10,235,175,117,254,254,186,239,235,180,150,239,151,239,239,123,226,227,148,143,83,60,86,36,198,10,225,234,136,39,123,123,84,190,189,94,107,106,28,
183,36,210,9,226,82,175,85,96,187,196,232,177,18,206,154,216,7,82,172,225,4,169,21,153,106,140,117,65,231,152,203,101,93,87,116,158,214,18,135,101,87,75,137,132,105,24,35,195,146,118,72,204,113,123,29,
126,190,221,245,93,247,191,127,95,47,241,241,254,247,113,158,225,148,7,114,185,252,62,99,21,106,173,8,177,98,51,94,163,167,8,30,163,87,42,178,236,169,211,121,128,100,67,108,44,39,213,52,217,62,215,154,
76,211,62,59,18,159,38,168,2,77,14,119,212,74,77,204,3,117,94,127,241,90,99,173,113,202,235,237,117,95,127,165,180,51,164,248,136,143,121,128,147,120,137,212,218,236,135,212,171,35,14,233,253,237,125,
254,246,63,151,42,85,62,78,120,59,45,1,53,60,196,169,53,62,62,30,49,18,235,112,2,251,158,168,52,190,147,232,75,90,166,69,231,90,227,16,179,206,211,186,175,195,62,196,165,172,187,218,158,32,201,35,122,
101,223,209,125,142,219,229,17,120,61,247,183,95,111,63,126,254,154,242,64,122,79,132,35,252,190,126,252,190,248,73,229,227,129,212,240,115,136,49,78,121,138,209,165,70,168,93,158,22,77,214,208,85,165,
90,218,83,244,184,172,251,62,176,39,153,134,184,36,83,72,85,34,120,133,39,95,33,117,79,166,251,206,180,204,203,84,9,143,148,230,184,12,242,113,97,72,239,187,166,125,223,117,142,203,58,83,171,196,62,131,
196,26,189,66,165,134,171,67,56,211,240,90,95,14,199,241,3,56,66,179,148,7,46,4,54,70,217,124,115,128,192,141,62,28,28,244,105,207,9,91,17,135,82,164,208,194,26,48,76,92,28,184,81,68,51,35,225,126,88,
96,188,177,81,182,27,161,197,146,199,232,227,1,4,228,0,184,171,57,185,69,113,237,175,32,208,221,37,195,128,44,42,197,178,128,150,130,60,99,126,53,16,1,119,16,121,250,8,226,40,106,138,122,161,0,119,67,
160,72,190,3,20,181,160,16,178,83,220,129,44,14,78,112,239,11,195,152,129,3,14,248,246,15,32,92,99,13,215,58,156,132,107,173,32,206,201,251,156,215,31,241,18,235,9,39,231,158,144,88,129,166,96,197,173,
68,162,47,63,46,121,160,166,232,145,143,95,147,239,186,163,111,239,186,164,37,129,253,152,242,20,167,60,44,127,191,26,191,152,46,63,127,105,29,142,112,16,94,247,87,169,149,88,63,78,78,241,112,138,51,199,
129,133,65,162,127,156,80,99,21,162,211,116,55,166,236,251,158,82,172,209,211,62,101,246,117,16,167,218,180,76,67,37,73,244,101,93,19,210,36,183,227,138,231,139,235,16,29,210,92,227,118,201,195,60,121,
172,84,25,222,119,244,58,197,117,64,170,120,170,178,93,62,206,83,62,30,145,88,43,225,92,190,252,251,191,254,101,55,70,182,17,127,186,250,2,219,33,219,97,148,237,144,238,204,129,247,165,197,1,217,24,113,
8,163,99,101,187,131,74,46,100,149,252,149,178,65,0,114,241,112,7,110,220,185,29,166,166,226,97,204,218,60,65,83,100,59,90,186,130,175,48,79,188,253,181,30,157,104,187,131,54,15,51,43,146,105,76,213,31,
109,174,40,124,198,145,157,183,232,1,111,214,39,252,79,231,21,104,32,102,110,163,203,118,200,54,122,147,8,113,161,125,12,7,16,14,236,118,136,255,245,111,192,101,99,132,172,132,209,197,101,59,90,4,251,
12,58,193,184,141,77,50,250,138,224,225,0,113,194,232,66,190,141,228,178,29,166,134,254,241,12,66,230,118,24,42,27,99,70,237,118,135,219,241,132,121,27,93,182,22,47,64,251,159,0,254,249,185,187,210,207,
136,211,128,146,139,75,6,74,254,76,60,125,134,215,125,216,51,171,37,46,0,219,232,86,92,252,51,113,215,54,38,128,131,169,108,71,56,16,111,203,182,64,250,11,219,128,108,99,214,111,255,4,23,191,94,242,16,
7,155,127,95,234,146,62,56,231,60,125,92,153,150,161,202,199,181,66,74,103,109,46,140,19,78,4,255,56,79,36,186,196,11,49,175,229,231,196,80,79,187,13,175,123,162,98,73,34,16,227,246,216,121,101,103,246,
225,136,195,158,230,159,232,124,169,216,180,166,48,252,62,170,248,9,49,186,84,137,68,151,90,35,46,149,24,29,241,74,221,19,166,105,223,247,125,79,166,105,30,22,6,97,152,135,125,45,181,38,89,18,144,234,
83,163,3,96,172,221,87,175,2,94,235,240,32,85,98,189,122,123,94,160,218,28,193,43,150,146,77,151,122,202,199,165,82,169,200,101,136,21,62,206,54,165,205,53,92,253,125,85,123,193,9,46,108,10,20,70,40,50,
34,232,118,39,115,147,22,187,65,151,45,113,56,4,92,14,194,147,13,114,1,200,136,113,239,86,66,67,187,196,97,170,152,234,38,77,108,183,114,179,12,220,188,132,227,24,9,30,4,54,111,19,57,14,238,193,155,162,
238,58,86,20,83,45,69,81,17,55,138,225,146,5,50,45,20,6,144,44,79,32,225,166,106,96,222,217,187,101,98,8,8,230,141,21,115,70,193,51,93,180,65,124,4,2,200,150,197,5,25,45,3,136,34,227,70,65,202,183,127,
70,191,198,252,157,247,125,138,144,167,143,11,126,197,229,49,103,93,126,77,249,85,92,98,181,148,44,197,74,99,46,113,42,92,137,64,158,226,58,77,76,185,196,140,238,251,175,183,247,132,196,11,228,97,246,
101,87,32,73,188,228,33,156,225,245,253,215,244,251,199,196,199,251,175,183,223,99,172,75,186,214,43,149,107,93,6,27,72,96,251,190,167,211,230,10,136,87,168,137,37,201,128,64,140,17,226,90,152,168,113,
168,59,36,82,28,154,217,169,115,115,75,45,129,60,32,97,36,136,94,165,134,83,214,125,159,47,224,9,176,36,62,79,31,215,60,144,0,146,105,141,53,214,250,113,202,67,34,191,127,184,77,181,46,205,9,175,130,215,
211,166,137,252,237,255,78,113,123,204,63,127,12,115,158,96,242,43,196,237,82,227,199,101,90,96,90,223,30,54,185,76,203,174,182,166,174,252,28,194,41,176,61,106,156,182,199,156,39,255,248,181,174,168,
221,126,233,185,239,111,191,47,108,143,36,30,94,19,32,209,107,77,246,202,73,218,135,51,122,124,188,189,14,92,220,212,6,106,69,60,252,125,29,194,181,98,186,151,65,234,236,96,41,86,219,247,4,137,42,53,130,
71,242,228,41,2,53,86,67,231,184,188,61,136,181,249,135,115,77,187,154,34,181,130,84,82,2,91,19,149,51,60,118,157,201,67,5,144,129,24,243,112,101,32,188,239,9,75,41,156,31,195,165,134,49,18,189,214,171,
203,80,177,219,195,18,64,116,75,204,228,105,122,41,217,239,178,21,132,146,183,13,201,158,253,200,182,65,75,80,29,220,178,129,170,105,49,16,113,8,193,14,1,63,16,242,33,148,44,71,225,166,112,20,51,202,56,
110,126,4,113,59,8,4,201,206,115,136,154,101,193,224,56,92,138,96,130,224,114,31,145,195,9,160,152,11,130,40,78,81,21,250,162,14,78,201,34,217,219,124,10,174,7,120,243,217,218,37,109,26,155,166,33,40,
106,193,224,16,44,131,96,33,4,23,3,44,59,114,191,17,80,108,100,28,145,17,119,68,216,90,206,238,153,254,203,168,4,114,97,251,246,207,117,215,229,199,118,193,227,250,235,215,196,244,241,42,113,152,127,254,
253,251,242,253,10,211,146,222,117,206,131,212,61,197,97,217,191,63,16,63,207,20,46,94,129,232,41,92,240,180,164,133,95,137,121,153,134,189,224,241,17,206,247,129,249,227,250,176,215,58,87,44,145,164,
134,243,227,76,41,69,214,31,53,156,54,193,180,14,84,88,52,66,149,122,38,108,216,83,173,85,28,164,198,88,219,42,136,139,75,196,231,188,50,215,184,172,41,97,107,162,235,247,4,212,174,226,195,105,36,168,
132,147,74,58,19,216,144,230,137,41,239,122,94,31,194,186,78,211,180,190,157,53,189,235,123,178,148,226,199,5,143,108,39,53,58,215,201,63,174,31,63,150,121,0,32,201,199,229,231,52,193,229,219,255,73,111,
239,186,252,200,131,77,195,126,187,192,35,92,92,152,126,254,154,30,53,194,186,151,143,71,98,217,213,166,58,15,239,169,233,173,139,3,72,126,187,94,182,7,188,189,223,94,247,57,50,229,121,248,120,8,117,56,
83,11,199,7,137,84,146,84,27,56,25,134,19,62,46,67,181,95,111,191,134,152,167,65,42,18,7,241,26,163,3,36,187,157,18,35,21,42,209,177,68,255,92,163,199,232,243,180,174,251,170,29,69,182,170,205,113,73,
88,178,125,170,72,133,147,189,233,162,150,86,11,39,225,21,91,135,232,251,237,186,76,203,196,247,191,127,14,113,56,9,239,106,26,94,109,159,46,249,251,89,227,239,19,217,78,150,239,239,63,182,251,52,185,
196,248,113,34,252,28,230,60,225,203,183,71,186,78,113,242,183,51,125,156,243,35,66,61,99,244,184,253,152,168,68,24,246,233,97,251,188,170,49,69,39,217,62,128,120,172,96,233,227,117,120,196,71,56,25,126,
253,136,223,127,78,48,44,243,67,188,202,67,42,39,196,237,186,61,170,196,186,188,189,2,225,184,126,156,156,21,210,252,83,169,41,70,170,120,165,74,196,145,138,237,229,33,180,232,12,27,32,117,126,161,138,
219,148,247,33,174,101,101,174,82,65,234,60,213,180,76,211,178,151,56,164,138,173,9,144,105,250,104,113,238,123,130,19,78,100,37,85,210,251,180,78,19,92,126,170,77,203,158,206,183,119,181,87,163,228,97,
126,88,170,215,24,253,148,56,61,102,46,19,44,195,50,188,39,162,207,203,52,229,233,227,199,95,255,110,142,154,161,4,142,48,110,35,224,194,118,136,11,100,212,84,200,37,127,150,73,62,29,62,2,247,238,77,183,
184,48,183,82,42,52,67,208,141,65,56,194,167,252,183,209,234,13,189,58,218,94,104,14,172,35,184,52,107,143,56,246,71,109,6,33,131,10,46,254,5,66,159,175,253,49,84,252,233,45,16,104,133,38,105,143,134,
150,86,87,43,219,152,81,122,89,72,193,110,163,247,175,226,130,183,9,115,113,196,195,209,64,114,201,197,95,92,112,17,83,197,56,100,220,14,96,99,219,70,219,104,202,19,112,69,13,140,86,15,117,64,132,48,82,
90,145,240,182,81,50,89,21,37,192,231,166,97,19,14,142,131,0,8,2,33,128,155,170,20,205,32,174,34,136,117,84,125,58,74,32,2,189,130,211,253,32,40,96,217,197,209,220,44,3,128,152,120,7,73,21,55,105,46,26,
28,7,79,111,11,224,160,160,138,21,191,111,168,116,20,42,132,114,248,115,5,44,35,136,139,160,46,210,170,41,155,224,134,110,225,175,127,111,207,66,134,134,251,13,70,216,14,176,219,1,2,153,226,86,182,177,
85,152,90,138,255,73,73,90,229,189,83,3,217,24,29,211,112,132,175,178,109,135,180,85,4,58,128,166,189,164,214,10,88,78,47,72,138,75,46,52,84,227,116,44,125,182,3,72,47,97,208,16,88,26,57,28,43,157,29,
192,161,23,242,157,134,253,182,92,231,94,23,151,12,133,231,36,183,255,228,248,198,81,183,241,57,159,139,3,226,178,29,146,53,140,223,254,239,143,143,215,29,77,243,247,225,152,47,143,235,118,225,33,85,166,
135,205,75,51,87,198,250,26,215,125,144,186,39,82,170,54,187,205,110,105,89,127,184,37,219,7,27,62,174,21,234,121,117,100,125,59,184,122,75,19,230,239,143,14,196,3,36,70,15,39,182,167,125,215,22,202,167,
232,149,88,89,208,88,17,23,79,53,146,19,181,198,42,75,74,41,165,100,106,115,197,18,209,227,202,138,238,251,62,199,105,88,6,90,206,41,45,115,183,204,177,66,28,230,166,236,90,122,10,120,123,103,79,32,94,
169,212,121,216,215,181,172,236,9,210,9,150,154,55,37,21,137,124,92,25,206,26,46,14,225,66,116,128,101,159,30,54,77,241,226,47,197,71,43,197,200,199,97,184,108,144,113,178,11,174,25,184,65,249,172,207,
63,131,11,205,166,166,154,77,237,70,80,238,110,6,56,182,21,2,14,4,130,51,210,71,151,205,3,67,209,98,128,4,158,250,73,53,120,155,88,112,47,237,163,3,96,214,10,82,106,56,148,162,88,81,205,128,74,47,232,
73,201,98,79,182,21,151,190,162,208,58,3,56,20,133,224,1,16,65,84,201,159,154,240,201,182,184,4,240,195,55,186,135,53,146,61,24,136,150,44,37,179,101,190,253,51,50,109,143,183,243,237,36,197,26,47,23,
166,184,164,185,198,33,92,39,175,203,43,115,222,117,129,180,36,146,165,42,195,146,72,123,218,231,58,79,75,249,253,247,61,201,247,115,47,213,18,188,221,167,199,41,75,10,199,181,158,204,14,16,206,240,56,
9,15,241,106,243,52,197,101,95,117,79,54,93,170,44,205,147,65,30,8,78,37,86,196,105,238,85,2,216,53,205,145,88,73,141,78,117,94,167,74,90,166,88,23,230,201,145,26,61,45,106,41,86,62,78,42,145,74,130,179,
218,0,112,130,12,192,53,62,8,231,50,68,42,41,237,123,71,82,34,188,90,2,137,126,245,88,145,7,88,138,85,106,93,148,51,89,138,145,41,79,83,126,77,246,215,191,196,173,108,135,129,130,221,90,149,223,90,15,
67,83,6,66,86,67,77,159,230,166,75,179,21,60,28,86,114,241,112,47,89,237,153,215,8,119,165,169,169,192,209,245,149,184,184,221,198,206,75,166,70,175,164,210,213,13,95,38,174,105,138,174,87,164,61,34,46,
153,63,30,111,198,23,125,42,204,63,56,203,195,209,115,101,93,239,58,32,189,179,201,74,131,160,61,221,135,21,188,191,136,184,120,179,140,128,161,16,70,104,53,220,111,255,136,113,157,30,164,157,125,142,
211,207,146,19,144,236,237,199,146,230,156,106,140,209,227,84,231,53,237,179,179,171,80,177,117,221,215,125,158,200,243,67,38,223,7,174,195,162,198,96,137,106,137,115,79,192,105,115,29,14,228,227,0,194,
131,143,51,93,113,97,251,185,239,106,154,132,154,106,204,235,154,104,197,220,161,46,115,125,210,149,165,165,101,102,128,156,108,90,166,181,224,209,171,205,209,107,157,193,211,60,69,162,87,104,206,186,
124,156,212,240,104,158,21,182,235,9,68,28,176,245,181,98,123,146,117,157,243,68,165,126,161,74,62,126,228,129,88,43,215,10,85,156,88,147,37,219,19,105,79,242,113,124,92,24,150,33,201,199,183,127,68,134,
10,166,59,83,158,190,255,126,181,132,212,116,198,193,166,1,162,199,232,49,50,197,33,198,248,253,10,49,14,243,48,127,255,145,167,92,182,179,86,222,78,170,41,105,79,123,34,188,2,41,12,39,164,106,175,225,
172,39,82,185,86,78,75,49,207,219,207,95,148,129,20,206,186,36,132,105,26,36,250,179,154,234,82,101,16,183,1,18,236,58,15,49,175,19,107,74,113,98,130,186,236,187,214,138,68,200,115,93,166,60,84,36,86,
134,19,136,221,19,110,35,165,102,100,6,75,200,74,194,80,226,176,79,195,178,238,9,164,130,237,73,162,159,53,217,236,141,55,165,86,132,74,10,175,123,194,148,229,85,30,53,46,37,198,252,250,18,64,48,52,20,
178,114,28,86,26,223,102,74,22,132,39,151,35,56,35,27,155,139,195,152,53,195,1,198,241,149,70,106,222,167,60,131,208,27,99,19,35,60,152,221,36,151,237,94,20,55,184,131,18,200,142,100,127,54,103,121,55,
214,106,152,129,154,147,75,175,131,56,158,29,84,65,26,76,46,133,210,1,108,235,137,219,211,21,4,56,68,164,199,214,170,160,221,85,52,85,69,196,197,40,165,245,137,60,151,7,16,220,49,59,236,217,128,228,98,
220,50,249,102,47,35,56,55,99,164,0,102,10,130,64,9,155,102,112,188,105,148,62,225,200,33,4,129,130,222,154,81,2,130,26,220,194,113,7,8,206,189,129,124,136,63,117,202,161,140,94,242,65,79,13,40,102,28,
94,4,138,21,4,92,26,65,132,79,103,84,77,20,40,217,36,127,126,21,105,152,43,130,227,46,190,117,208,112,15,170,246,103,180,240,196,132,137,201,179,49,199,75,187,38,208,178,123,64,171,39,53,253,150,17,67,
111,65,49,53,163,24,168,31,197,203,200,183,127,230,117,174,239,106,223,47,12,54,77,223,223,167,143,135,120,56,227,227,61,165,102,159,162,199,246,23,128,26,185,120,132,101,190,12,242,113,141,57,201,112,
146,236,237,24,134,87,44,217,43,180,144,44,92,137,254,153,12,152,115,138,131,21,239,245,63,210,158,254,87,53,53,86,169,181,98,59,59,201,148,20,35,224,243,186,50,251,219,57,183,252,95,30,176,183,31,208,
74,161,241,33,177,182,78,20,206,110,67,251,168,80,9,215,88,247,105,29,36,107,178,189,228,125,221,147,68,175,203,180,150,137,233,171,122,219,108,113,125,59,235,158,56,79,140,189,12,54,13,54,197,184,12,
212,186,255,245,87,105,241,31,221,22,6,70,71,28,113,201,106,189,203,168,209,29,188,251,49,237,118,233,151,221,110,119,40,219,33,219,189,180,52,180,176,253,217,94,219,12,106,201,42,110,95,157,130,148,92,
224,41,101,109,124,53,191,154,118,203,101,197,63,195,194,102,49,101,235,205,80,253,31,239,207,63,125,166,255,30,146,161,199,184,173,61,209,1,211,102,249,242,103,173,227,185,235,246,173,69,133,118,187,
107,24,113,172,100,94,116,219,202,141,86,192,28,75,230,184,103,233,160,23,84,27,36,238,159,219,145,231,212,64,134,141,205,237,118,40,228,67,252,208,124,71,66,96,219,14,54,24,189,133,93,158,161,55,65,55,
7,215,0,45,168,187,103,117,112,17,4,132,103,179,104,115,70,155,244,185,56,20,204,179,169,59,18,124,20,128,128,187,227,77,214,76,249,127,225,202,114,161,208,154,29,21,133,108,64,145,150,134,124,54,44,10,
46,184,101,197,204,128,67,36,103,244,192,216,48,33,107,249,246,63,199,149,203,146,146,41,251,196,180,164,183,95,107,2,27,136,57,33,228,20,189,34,209,137,30,99,132,90,35,177,198,200,80,231,237,250,136,
191,207,253,21,211,148,168,96,186,167,58,28,225,184,198,229,135,11,177,34,49,86,25,230,69,151,219,213,165,146,76,77,247,93,247,25,151,24,227,32,181,198,232,181,209,117,73,36,144,154,210,60,213,26,135,
200,84,137,53,14,53,165,150,227,147,252,218,21,194,239,203,199,167,237,251,15,241,251,143,33,117,30,226,148,117,142,212,69,197,105,125,70,251,58,236,26,89,39,159,227,146,128,10,75,226,237,215,190,119,
167,37,175,229,251,251,92,211,254,235,117,97,45,107,90,190,253,74,31,143,133,185,206,203,219,235,242,253,50,216,143,149,61,49,127,92,125,94,82,100,138,46,177,214,86,28,174,17,170,176,93,162,215,24,107,
189,178,76,15,153,150,148,44,17,174,113,81,18,225,176,87,106,157,93,188,198,42,248,199,149,232,115,77,87,23,199,82,50,53,77,204,190,164,26,161,198,24,113,132,88,171,13,9,144,26,43,226,109,201,216,100,
99,79,72,5,36,50,45,251,16,33,15,103,124,200,210,67,59,104,106,43,156,77,39,62,175,74,141,145,232,113,138,224,36,91,19,212,121,216,117,47,113,176,117,213,202,179,158,102,105,79,188,107,218,247,29,125,
123,93,40,249,117,102,73,111,191,246,242,253,215,52,24,47,37,28,65,113,115,189,135,102,208,181,73,73,150,172,173,211,185,75,134,187,52,25,244,195,9,108,136,224,10,100,21,81,179,59,148,64,16,4,48,114,112,
97,3,24,29,183,205,196,37,243,41,134,66,171,54,103,1,119,4,207,136,2,136,63,69,222,29,201,24,6,69,196,49,17,240,182,38,42,182,5,148,166,254,90,111,42,225,222,146,77,253,170,8,100,119,204,61,231,77,160,
168,25,56,42,228,108,5,12,196,200,77,3,0,173,234,118,227,160,213,215,179,6,20,238,108,129,242,226,112,160,129,96,101,84,205,66,166,128,113,148,172,161,136,225,109,42,233,82,221,244,11,135,140,64,43,37,
40,238,82,0,242,93,14,238,174,65,10,77,221,9,4,199,77,239,165,99,74,251,161,135,172,128,184,58,46,157,30,29,71,0,130,120,183,237,74,161,144,157,166,226,192,138,228,27,226,229,104,94,130,26,28,22,64,56,
84,123,228,141,34,230,158,81,131,98,20,238,79,127,78,218,237,242,228,11,53,109,141,239,138,33,138,48,90,131,194,41,118,191,163,112,187,223,241,23,57,4,24,245,94,112,9,154,165,128,66,198,85,238,205,82,
21,147,190,111,241,231,90,180,254,26,119,1,19,140,141,2,183,91,22,39,200,72,22,14,65,142,205,125,148,230,26,222,130,3,70,183,133,42,189,140,227,226,222,173,108,155,89,228,57,179,146,139,128,227,185,116,
215,183,136,20,92,239,128,75,247,64,21,40,247,158,230,123,42,122,49,138,161,38,106,222,242,178,166,96,165,153,16,71,243,147,47,40,70,215,242,218,249,225,57,171,110,5,8,232,189,40,246,146,45,27,1,195,93,
252,16,205,144,81,109,44,79,201,217,220,21,151,190,4,159,155,58,158,156,147,203,134,230,67,201,62,142,186,137,54,207,83,2,226,114,116,166,177,176,141,16,20,176,110,185,188,240,41,112,226,226,93,252,17,
58,85,16,121,186,46,166,14,72,111,42,114,123,190,196,157,62,193,39,146,62,71,201,168,161,153,98,64,166,96,80,242,147,218,20,41,230,10,25,74,75,216,210,68,37,231,30,4,4,24,29,181,3,200,192,75,81,69,200,
69,67,0,217,130,102,212,8,224,228,226,20,69,68,16,127,46,241,41,43,2,108,184,163,219,221,68,45,120,49,39,140,24,228,13,223,70,60,184,108,66,35,205,225,119,57,172,103,213,213,232,66,141,243,57,119,243,
246,114,91,162,113,28,128,27,55,160,128,228,230,40,104,67,30,238,138,25,32,238,116,147,15,136,128,60,177,85,204,53,155,150,140,146,159,58,19,48,26,50,187,222,106,71,109,4,144,27,99,49,131,114,39,96,5,
21,55,85,202,237,37,27,154,71,165,199,87,247,198,187,163,98,185,56,217,69,61,63,161,1,233,251,242,134,180,17,16,59,84,115,208,195,178,10,71,222,8,112,4,131,28,70,241,209,101,243,67,0,52,91,3,79,191,56,
163,125,32,75,147,58,204,40,25,113,196,13,147,39,105,238,214,144,87,204,104,162,12,210,176,162,10,150,13,208,167,86,247,167,80,25,106,94,154,107,253,39,182,68,0,47,185,41,51,50,185,168,32,144,113,242,
189,128,178,9,42,135,107,54,50,197,132,124,255,246,239,193,118,93,246,169,158,54,213,120,89,247,105,144,154,170,12,187,86,100,138,142,76,30,97,73,17,34,56,17,98,140,49,214,88,99,140,117,81,169,86,126,
206,245,237,87,90,6,43,151,233,231,144,228,247,223,71,157,31,203,32,78,124,32,206,219,123,34,237,218,124,165,207,238,130,138,84,137,94,103,167,2,178,80,38,6,42,196,56,249,204,180,172,83,92,152,134,189,
61,81,211,190,39,18,86,106,3,103,217,19,225,220,81,192,246,219,171,205,17,22,69,60,226,36,219,213,134,100,211,96,19,211,146,118,134,22,119,177,204,131,248,188,76,131,77,211,202,202,20,93,168,113,114,216,
139,199,237,186,254,90,167,186,36,169,105,174,243,199,239,215,101,47,195,139,115,35,52,148,103,60,23,50,27,136,119,167,30,196,249,15,246,109,92,245,148,16,4,113,81,199,25,53,168,225,158,85,224,64,240,
160,32,226,4,39,28,106,112,11,102,6,198,237,201,25,66,83,239,32,77,215,122,99,11,105,242,150,181,173,100,205,235,6,26,92,138,60,217,59,112,160,205,254,233,33,5,154,63,35,253,113,41,70,201,104,6,165,96,
88,110,186,171,171,73,205,40,165,52,173,236,136,80,50,140,91,43,107,132,140,109,248,120,167,148,204,139,201,253,118,216,173,25,5,209,76,177,195,0,19,164,21,246,4,35,243,25,37,126,130,1,96,228,175,110,
83,59,80,13,40,110,200,184,153,140,8,190,193,248,44,28,30,199,13,64,185,151,62,147,123,147,137,46,147,93,201,91,3,219,189,72,129,162,148,2,102,96,166,166,168,25,237,60,31,170,127,36,25,164,79,216,40,236,
136,169,102,239,146,152,189,17,95,11,228,156,53,75,219,73,201,189,159,20,172,145,168,24,62,150,92,110,112,32,122,199,54,181,45,23,190,253,53,164,225,58,93,190,255,218,83,28,150,148,210,162,86,150,169,
166,56,145,167,232,113,202,67,74,189,31,201,107,172,68,136,84,98,141,117,209,133,31,78,13,195,107,115,128,195,251,43,150,72,53,250,240,195,63,174,30,227,239,225,234,39,64,178,4,231,174,150,44,165,24,193,
35,14,53,58,21,98,141,53,85,98,77,73,226,20,99,141,84,75,17,71,92,242,80,231,97,103,79,111,191,118,181,100,148,60,204,213,82,237,88,10,39,32,113,187,120,149,232,21,155,162,199,202,194,28,167,37,117,105,
91,210,190,82,150,125,152,135,29,118,93,38,23,71,152,226,118,138,83,99,92,191,159,32,48,229,25,79,241,178,168,221,46,53,241,118,159,135,51,45,233,133,192,8,220,181,0,197,62,177,45,228,166,106,189,152,
72,201,237,106,31,14,130,32,106,90,50,95,185,190,214,150,34,210,77,255,177,137,115,140,155,60,223,11,255,139,51,158,140,250,124,197,161,169,89,161,251,63,93,27,56,90,212,14,213,230,119,20,92,180,49,161,
17,14,67,4,142,38,208,159,231,161,20,199,213,92,115,147,54,45,56,197,178,168,130,125,121,17,135,56,130,163,159,37,84,119,195,81,97,204,8,35,27,70,225,165,52,75,85,120,130,87,76,51,42,120,241,92,178,27,
222,108,201,83,111,53,11,37,157,225,133,91,59,133,42,69,14,201,101,12,71,201,46,91,144,195,1,151,173,233,47,2,168,29,183,155,169,97,148,236,5,195,69,196,255,112,66,219,112,156,76,147,165,130,210,68,80,
26,54,81,176,167,220,136,4,69,185,83,4,178,119,156,155,62,149,160,10,52,247,129,146,77,36,23,195,11,217,80,200,165,41,78,239,42,64,242,243,77,15,246,76,183,122,177,45,56,99,80,115,121,105,220,226,222,
124,3,47,185,249,29,25,65,10,152,154,208,8,254,196,86,39,63,253,241,209,213,229,192,113,28,252,142,171,49,182,172,214,200,54,130,184,11,7,160,118,28,127,112,198,205,112,239,212,253,156,18,144,238,181,
63,219,106,115,46,5,203,254,185,190,58,16,28,237,60,93,4,220,213,186,171,161,226,136,227,6,56,210,60,8,138,102,138,43,100,237,206,89,126,170,97,23,23,36,55,183,86,128,81,239,96,136,171,185,222,15,138,
31,64,126,1,90,240,135,32,109,251,218,36,16,207,174,80,50,217,27,41,154,197,228,147,200,0,173,208,217,39,192,67,9,112,107,34,38,155,203,29,217,182,206,237,4,8,40,216,205,157,206,113,45,254,19,248,156,
83,128,126,44,160,69,48,74,222,74,211,252,10,79,185,68,144,103,148,210,76,102,41,109,22,233,110,173,34,128,131,182,253,40,14,106,42,45,120,44,228,220,216,68,92,124,219,20,3,113,220,201,65,9,183,140,160,
88,33,184,96,192,183,127,217,196,176,164,26,107,141,80,137,211,58,15,115,158,166,60,48,47,41,9,83,86,210,146,230,150,186,89,102,106,220,206,232,177,18,35,195,82,92,34,85,32,186,149,248,243,251,1,103,66,
170,205,27,195,101,253,126,121,156,189,14,30,142,212,122,30,210,9,225,172,164,138,120,154,243,32,142,248,51,171,82,35,192,176,79,185,248,158,36,18,135,244,190,162,61,255,107,74,43,181,85,241,154,90,255,
86,5,36,2,30,171,165,56,72,116,137,21,162,35,85,42,243,50,13,203,158,170,84,75,251,236,115,92,203,176,175,101,101,168,18,93,136,196,203,67,226,84,109,138,46,85,134,247,148,222,127,149,60,216,158,246,53,
157,84,210,62,15,47,161,137,151,61,169,15,138,120,193,139,137,23,17,114,86,131,242,12,12,212,17,31,197,5,17,119,164,120,147,122,220,81,71,71,33,0,91,40,112,140,220,198,86,200,15,33,112,52,169,105,156,
113,32,244,184,164,133,182,210,252,111,164,201,146,41,100,43,173,199,50,232,179,212,66,208,167,238,108,172,168,52,149,41,14,184,184,61,127,173,33,131,184,52,17,67,91,104,152,173,88,243,168,160,64,193,
196,251,198,93,28,183,2,226,70,11,52,90,94,164,244,104,189,228,240,114,40,142,53,195,1,130,55,57,119,67,17,39,187,98,106,125,57,12,4,23,92,192,27,142,192,3,27,46,32,184,206,231,102,220,0,0,32,0,73,68,
65,84,57,114,32,118,140,112,224,121,132,187,32,140,159,69,151,182,233,0,110,228,156,63,221,91,71,84,172,237,188,171,119,5,176,236,28,244,3,216,93,73,169,53,74,201,147,124,159,123,110,239,136,183,15,226,
13,11,157,216,246,60,155,129,102,105,241,143,11,142,179,57,46,46,197,125,203,82,50,65,45,20,204,84,179,223,236,16,35,100,142,151,22,215,126,21,219,26,155,100,74,22,167,41,71,76,13,105,188,229,60,41,3,
210,104,39,50,126,214,7,251,254,40,146,51,64,217,124,43,28,222,154,204,159,79,104,175,87,25,170,61,216,147,70,132,162,228,222,157,142,32,100,138,98,38,255,169,38,69,74,118,17,113,172,201,68,71,149,139,
183,24,146,70,59,119,225,9,172,102,10,185,57,72,217,154,47,148,41,210,246,112,128,108,0,97,44,25,181,163,97,229,38,148,112,20,1,59,138,242,237,95,13,234,95,3,82,107,164,34,117,217,167,193,134,20,163,47,
83,157,151,148,246,93,151,137,97,73,105,79,54,69,143,177,229,131,107,141,145,90,35,80,107,133,24,171,68,95,82,170,181,166,244,113,253,56,227,131,247,161,218,236,194,51,95,30,78,72,54,87,76,83,178,29,253,
172,66,11,30,107,156,156,148,106,37,50,177,234,50,200,176,179,238,195,199,249,71,13,185,214,36,64,100,48,250,193,57,128,26,23,37,214,24,157,22,94,214,218,42,113,177,198,58,47,195,188,238,211,162,209,211,
156,167,97,153,134,121,153,188,70,34,14,226,167,120,140,151,252,253,71,222,213,82,24,115,74,103,21,63,109,136,12,111,103,181,244,34,98,102,69,213,154,95,39,210,123,108,12,207,148,252,228,173,146,159,78,
24,254,135,79,228,93,136,0,17,186,113,18,1,132,131,195,104,28,153,165,157,210,3,56,2,205,201,85,12,45,165,203,137,54,245,33,184,153,33,34,56,148,166,1,10,216,29,113,49,180,215,144,123,236,136,170,118,
135,193,221,197,181,233,169,6,70,123,194,1,4,122,134,161,233,164,146,41,91,115,230,182,173,189,30,130,195,70,185,55,222,63,184,25,98,116,145,57,12,236,5,20,205,180,184,177,207,237,37,83,154,150,203,93,
217,121,105,50,89,4,62,101,194,64,164,125,17,193,3,69,124,107,145,150,32,2,55,132,140,200,45,123,135,216,140,81,220,253,51,131,219,149,60,40,184,131,67,107,126,111,154,203,52,155,121,81,48,23,248,204,
88,57,29,13,221,159,254,162,31,157,90,32,207,157,254,225,67,102,250,47,84,24,99,219,222,120,108,2,34,227,129,248,152,185,101,212,184,73,30,213,40,30,80,3,51,165,240,237,255,44,105,94,216,83,156,134,101,
106,133,97,137,76,121,138,75,154,243,60,229,68,178,157,25,143,78,90,38,168,210,204,123,141,3,54,209,190,120,173,242,88,166,237,122,169,177,74,172,17,112,134,139,199,33,213,143,113,250,24,134,225,129,165,
121,90,134,10,240,231,185,137,86,133,94,147,44,235,0,169,75,149,239,147,207,211,162,243,186,175,243,144,218,73,230,94,67,174,72,69,60,86,250,195,85,28,113,144,60,147,135,88,161,29,102,145,10,66,21,34,
181,206,25,221,215,169,206,94,227,176,12,243,186,147,106,172,87,39,60,62,30,18,157,56,229,31,131,205,67,56,226,148,211,156,167,122,205,233,237,231,52,173,251,236,111,47,168,53,51,231,77,123,187,136,59,
104,118,109,185,13,3,208,118,87,20,71,188,179,182,139,224,120,39,31,46,202,225,217,92,112,193,29,9,35,52,203,238,247,113,60,68,254,232,46,8,141,63,219,219,98,168,42,78,211,230,64,118,164,128,83,44,171,
246,0,197,21,199,75,243,33,68,96,67,192,221,241,39,19,53,11,227,125,17,105,102,251,15,38,12,90,92,92,112,43,230,133,98,109,55,7,35,184,72,19,42,15,135,108,220,66,214,220,212,190,103,197,229,120,121,58,
5,77,222,160,117,72,102,105,168,42,217,212,186,92,72,63,102,229,210,192,113,3,37,211,141,191,116,136,84,186,255,238,28,14,185,16,238,217,148,108,153,91,41,91,49,11,16,142,230,122,180,185,218,155,134,242,
105,15,45,147,93,0,85,51,85,204,180,136,109,210,246,36,2,238,50,62,23,71,58,99,185,52,247,232,83,80,59,108,125,232,33,136,203,102,166,157,25,60,60,31,112,113,144,77,13,142,224,35,199,1,37,31,202,216,124,
133,204,75,211,118,10,38,148,220,117,130,20,239,26,75,65,133,78,32,125,158,74,19,4,90,73,68,249,52,12,66,54,43,157,184,109,180,172,116,41,180,206,136,3,31,253,198,248,204,83,56,120,115,117,225,139,41,
138,27,165,148,92,164,101,252,138,154,41,88,118,109,237,123,89,0,111,116,147,78,63,105,19,217,215,218,8,79,52,121,103,61,23,113,92,252,40,136,160,57,211,14,103,75,179,11,226,112,80,92,66,171,179,168,109,
5,130,151,187,130,105,248,246,239,37,197,41,167,121,101,157,153,166,53,205,209,169,85,98,77,203,60,229,148,108,206,115,21,175,72,133,196,50,212,26,157,8,195,174,75,66,150,117,2,143,45,141,155,222,30,226,
205,88,87,106,24,222,167,229,237,239,99,25,246,97,95,251,25,247,229,199,148,191,106,237,49,146,181,29,254,178,4,72,140,16,87,141,48,253,113,146,121,223,53,205,83,181,129,88,197,83,141,30,99,109,33,90,
172,21,1,34,53,137,207,237,148,28,226,80,105,127,137,21,34,30,46,53,186,16,169,226,41,82,153,87,24,0,241,90,109,176,153,143,11,149,74,252,125,221,46,177,194,252,104,7,184,223,78,118,181,95,47,89,109,107,
254,109,201,128,26,142,9,238,72,211,88,154,181,201,137,11,32,197,58,44,153,210,106,197,253,166,187,96,28,96,46,34,16,16,70,205,165,247,238,3,217,81,192,253,243,20,23,184,123,243,77,65,193,44,243,233,142,
120,193,159,43,151,130,229,44,255,171,214,40,222,152,222,121,202,227,127,240,22,127,72,96,227,102,1,188,189,40,208,243,12,222,160,197,71,28,33,56,62,186,19,196,173,73,224,97,162,80,94,48,189,131,58,154,
53,3,184,200,115,21,205,168,209,42,42,72,131,187,11,139,120,51,12,224,197,178,131,11,46,138,52,37,214,32,63,90,248,37,173,91,205,138,168,57,10,246,233,184,27,88,207,152,152,209,188,24,49,0,71,158,27,21,
105,235,246,160,21,239,151,115,119,9,250,227,242,223,26,171,63,134,100,19,201,205,134,129,59,46,207,57,10,32,30,130,220,225,214,22,205,134,28,226,32,35,161,28,119,49,15,20,135,146,95,10,86,54,16,3,43,
57,163,56,234,93,117,169,161,205,203,145,206,88,109,143,8,130,230,166,141,81,196,32,35,14,46,52,239,169,157,206,23,135,205,65,115,81,205,72,145,77,158,85,100,161,39,228,77,181,148,86,51,84,43,120,145,
98,25,60,103,227,217,223,128,53,254,116,1,112,28,249,83,55,126,49,22,252,7,115,9,237,193,92,188,79,5,34,56,146,157,173,131,56,226,220,184,187,192,134,50,18,60,32,100,39,115,203,133,49,100,51,40,47,64,
30,51,173,169,180,96,242,103,198,16,53,83,115,192,27,5,51,25,52,3,25,138,119,184,138,127,193,231,95,91,144,134,225,131,226,20,8,197,113,14,151,123,49,81,196,57,248,242,77,213,232,149,106,201,45,45,215,
122,63,149,255,58,201,44,125,242,86,107,148,79,63,149,198,49,157,177,236,249,148,195,51,131,81,178,119,83,224,34,94,204,70,129,224,4,63,228,118,108,69,114,96,4,221,56,56,100,219,212,142,155,81,28,142,
27,24,188,100,165,37,88,143,76,201,20,183,63,50,134,0,82,12,49,107,87,10,24,148,150,107,118,201,20,44,139,163,80,196,123,182,4,144,158,119,23,114,193,92,132,126,193,193,11,229,41,235,208,187,11,64,181,
103,101,81,39,35,222,132,92,186,166,227,171,10,141,32,130,23,199,197,159,110,23,240,57,231,231,16,111,219,144,62,135,101,92,92,54,33,139,55,162,143,128,176,141,140,219,198,221,33,132,209,67,144,109,28,
81,27,111,135,35,110,71,225,150,249,246,239,133,178,174,76,149,121,93,53,45,187,38,44,209,138,148,178,104,90,134,100,195,188,38,89,166,74,28,118,246,20,135,101,206,201,102,166,101,26,210,156,147,124,188,
218,196,199,187,86,90,212,90,207,90,197,107,141,158,170,12,225,81,151,31,126,198,90,145,10,53,178,12,203,32,177,249,215,209,169,53,198,184,176,107,140,121,98,217,203,176,175,173,247,179,214,222,109,213,
142,180,71,170,196,72,244,90,197,65,242,92,99,125,162,38,66,179,179,179,247,228,87,172,203,16,97,136,113,157,191,255,156,166,105,221,247,239,103,61,107,28,150,180,167,84,17,135,51,198,159,83,220,198,223,
63,214,183,135,60,134,203,7,199,7,23,135,183,199,240,62,215,143,247,178,76,223,239,183,159,47,89,111,185,192,179,23,130,98,38,5,107,42,50,35,82,140,98,168,81,178,224,79,222,202,152,118,137,204,106,62,
130,51,54,8,29,186,94,232,148,181,67,172,184,73,187,64,155,165,124,150,119,28,65,28,74,41,226,205,249,113,233,170,215,161,179,181,98,165,131,232,222,153,72,188,185,227,125,100,178,118,197,160,207,43,197,
26,64,133,123,183,198,119,3,60,244,26,147,3,226,20,24,25,183,114,15,110,227,118,140,140,108,192,193,113,195,238,234,154,209,59,47,48,66,185,113,180,21,52,23,114,59,39,226,160,184,251,87,45,41,35,134,54,
125,85,154,206,42,128,21,107,134,129,175,64,23,68,164,233,114,167,89,19,135,224,237,110,70,122,71,87,19,204,231,7,4,41,20,21,190,122,63,159,135,29,155,225,112,193,121,70,56,128,60,63,0,207,78,90,121,94,
145,166,111,27,210,11,106,112,43,96,134,28,98,220,90,192,14,184,195,134,143,91,57,164,56,193,123,254,45,192,61,171,138,80,224,198,11,66,201,244,198,127,131,226,168,149,220,234,74,168,8,205,125,232,126,
132,102,26,236,142,101,50,96,250,101,24,154,178,147,142,1,199,15,65,172,140,158,155,237,106,63,178,73,214,252,121,102,178,15,121,242,17,185,100,218,9,204,182,175,94,133,230,63,106,141,210,222,249,68,11,
240,105,42,190,134,139,202,147,199,220,49,205,118,111,25,140,77,54,184,139,243,76,122,227,247,28,56,156,45,115,63,112,252,104,205,132,165,132,192,6,135,141,229,165,228,13,114,115,153,10,8,86,232,187,119,
163,9,137,218,127,32,172,24,217,77,233,249,230,22,68,66,86,12,149,79,167,18,92,196,201,183,134,10,160,113,154,80,76,189,252,7,182,28,64,132,236,148,140,180,128,187,237,139,150,169,53,190,106,141,120,147,
61,127,10,97,23,61,71,64,201,125,82,193,205,165,209,69,13,45,86,10,173,196,122,223,232,217,245,67,128,131,45,20,29,55,105,13,221,38,240,121,84,217,199,145,209,64,51,127,253,219,159,63,152,106,37,247,94,
113,74,86,33,163,72,103,114,235,63,98,68,83,67,249,137,35,21,71,50,138,161,96,45,202,20,62,135,11,108,28,194,118,64,56,16,111,191,243,83,254,112,12,219,198,158,251,118,201,64,247,232,65,58,85,192,10,222,
191,138,11,222,244,223,31,115,8,185,31,136,253,252,222,143,128,209,102,16,218,143,8,117,112,197,219,127,64,63,11,189,221,111,28,152,218,109,116,16,188,253,144,145,221,198,237,174,132,251,139,83,178,102,
123,2,100,184,62,91,79,181,255,2,170,208,185,138,158,62,65,209,46,125,25,218,182,10,72,241,103,253,18,7,220,93,200,48,138,115,23,225,16,92,14,55,167,228,134,147,175,225,230,79,142,40,170,88,209,255,103,
21,26,218,73,102,239,148,251,28,25,248,124,5,232,206,245,83,143,26,168,222,237,214,50,24,193,114,216,12,28,130,56,7,130,143,140,135,8,220,142,102,239,66,184,103,67,110,99,30,111,198,88,94,228,79,133,68,
15,121,192,41,134,20,164,209,17,205,148,220,179,169,165,25,153,162,77,120,27,27,100,193,179,136,137,106,171,113,122,155,70,243,157,44,94,240,32,14,25,233,191,199,221,149,73,31,95,85,104,71,40,100,254,
255,170,208,120,43,197,105,105,233,27,183,47,246,180,254,208,243,130,56,80,252,255,235,235,109,209,36,201,149,117,221,119,101,31,223,207,227,164,71,80,64,5,180,192,1,69,131,20,248,72,92,96,27,198,8,26,
196,25,130,6,161,195,22,136,89,36,41,96,36,15,48,26,35,104,224,5,68,54,168,17,56,9,112,129,201,35,178,214,62,247,170,59,51,43,194,229,250,49,217,191,153,164,105,255,228,190,159,149,110,239,233,193,216,
177,211,12,162,156,34,57,23,119,107,138,48,222,87,219,209,61,78,123,183,222,120,111,23,78,230,240,199,255,115,184,12,185,6,117,219,250,173,150,101,171,212,155,249,199,194,173,50,20,148,113,189,157,151,
27,253,182,120,45,139,47,215,143,43,165,157,151,173,114,171,215,1,245,182,213,58,202,153,50,182,143,90,198,16,101,168,112,235,75,57,135,159,131,239,173,122,221,106,33,212,204,175,229,57,77,18,93,202,249,
246,113,229,227,218,22,198,117,241,215,78,102,248,183,157,204,67,153,240,166,178,60,91,240,206,82,253,169,118,233,246,177,157,111,87,74,99,163,122,205,109,64,63,211,131,129,109,253,215,215,245,103,89,
6,160,251,99,221,249,254,235,75,249,175,239,231,118,29,235,99,251,138,110,231,159,186,127,103,140,235,253,111,251,177,12,174,237,252,6,211,101,72,8,58,205,60,192,161,167,176,126,166,42,90,195,172,153,
247,131,169,7,214,232,46,243,200,120,159,107,110,115,99,82,131,18,129,90,243,126,119,209,133,119,111,161,134,108,18,213,44,179,11,123,218,112,255,159,59,153,5,49,13,193,20,159,46,185,187,27,160,30,83,
87,112,58,61,44,192,236,101,26,132,67,200,140,149,134,159,28,177,174,207,163,160,224,116,15,238,235,73,23,192,218,193,226,79,172,221,1,218,91,3,91,233,205,28,112,58,110,210,75,44,62,173,17,232,142,32,
93,57,22,73,124,169,130,17,150,150,178,103,165,238,141,16,4,184,35,232,156,122,208,232,253,32,119,126,47,185,34,14,135,13,7,188,162,208,216,164,70,41,8,38,223,202,191,78,3,235,93,132,7,244,233,180,72,
161,3,200,253,211,54,32,65,64,223,225,100,244,104,235,251,41,20,236,218,153,59,178,78,39,216,209,157,203,59,52,86,8,123,167,59,88,127,3,216,91,75,245,242,208,33,64,17,189,229,150,135,228,155,16,221,163,
187,2,115,164,148,60,169,72,224,74,181,163,67,143,166,150,222,102,20,97,16,214,27,13,143,78,107,116,89,228,129,73,159,138,166,46,160,192,8,146,89,233,25,133,6,192,124,242,238,128,16,50,133,7,152,17,150,
48,52,199,195,231,196,26,224,10,90,199,172,43,48,232,222,18,151,48,187,207,45,22,40,82,89,14,193,233,126,129,184,227,247,147,239,151,150,217,51,171,104,224,1,111,150,235,129,7,51,1,218,91,42,235,19,1,
82,86,7,40,186,67,115,100,126,88,247,135,10,22,10,20,6,45,210,187,141,147,38,175,53,247,214,163,183,14,65,239,17,137,144,252,94,60,72,178,23,78,239,184,43,62,11,54,164,68,184,64,64,74,124,115,220,159,
219,187,156,84,251,103,99,138,78,19,151,84,172,133,2,204,154,239,192,42,118,129,113,234,0,218,89,239,4,180,211,78,91,223,233,239,92,214,189,59,172,43,123,116,238,116,135,183,196,105,51,115,58,76,127,124,
227,216,9,2,230,158,157,65,244,192,166,226,136,2,130,228,91,201,14,220,67,120,87,42,93,28,200,99,157,38,58,4,52,4,240,180,51,102,249,108,195,49,109,184,54,55,14,77,82,142,8,251,205,188,20,57,62,127,186,
37,80,155,147,112,97,77,65,79,175,66,64,11,136,128,139,249,123,112,10,41,172,59,51,219,120,213,254,14,247,232,109,13,246,203,138,181,83,206,98,223,129,59,239,214,46,206,31,255,107,185,93,71,25,26,71,2,
244,96,235,31,27,182,157,7,120,173,212,79,249,72,229,99,179,173,50,42,190,112,164,243,44,245,86,189,199,168,85,81,235,237,99,179,234,253,227,131,170,113,219,182,62,202,146,241,70,216,248,88,240,154,94,
241,87,209,226,203,118,102,40,70,225,252,97,126,30,207,243,180,103,172,17,128,90,70,122,213,203,128,49,240,20,108,101,241,165,44,3,95,54,99,108,31,118,179,219,121,108,231,81,11,16,162,196,184,254,250,
137,6,60,180,108,219,182,136,192,63,122,92,219,95,237,26,124,15,174,183,111,124,111,253,231,250,149,239,63,111,245,122,63,125,124,100,127,26,143,235,71,173,223,151,219,91,96,225,145,89,51,88,11,92,96,
152,19,158,235,228,201,164,9,0,67,221,133,171,3,145,42,152,99,71,142,30,78,199,80,42,169,96,88,16,137,87,238,88,239,192,193,124,158,229,105,195,249,36,252,87,20,26,250,39,245,53,0,4,129,31,177,70,167,
53,79,217,147,44,182,3,178,38,107,18,52,252,14,58,36,91,242,171,139,223,3,46,214,68,199,238,192,170,187,237,239,116,124,95,97,237,206,222,166,68,19,18,184,26,188,249,164,151,148,134,93,13,172,201,4,110,
233,73,128,204,71,146,32,68,195,154,172,53,28,34,73,39,31,128,119,159,129,254,222,48,11,210,180,125,69,86,129,23,220,95,223,40,109,184,164,173,232,47,27,46,57,91,143,151,59,84,243,247,49,180,110,208,141,
126,152,234,82,152,247,150,93,180,70,15,78,68,236,211,219,184,7,176,210,223,29,78,234,13,238,190,163,56,113,34,37,214,186,139,29,187,211,137,75,131,156,164,1,178,183,11,110,201,29,5,22,109,74,191,28,74,
130,241,96,198,161,32,58,65,15,159,42,24,128,183,16,30,200,167,164,80,35,185,83,120,142,209,60,90,200,108,70,26,83,25,159,69,208,154,76,164,39,223,130,160,183,25,133,6,20,4,253,233,78,136,228,149,242,
100,88,4,102,28,130,109,194,210,2,153,43,71,46,72,92,14,221,129,149,149,253,132,153,251,157,232,112,66,107,136,8,210,56,61,229,118,254,147,71,247,189,7,51,245,12,122,248,219,238,150,104,149,34,4,128,84,
26,14,24,249,211,166,8,145,98,208,59,22,79,188,235,30,29,136,14,45,169,178,65,247,64,157,22,225,157,206,133,6,169,48,28,72,2,144,217,31,61,28,68,0,73,79,51,10,13,242,230,185,127,97,98,167,8,129,183,20,
132,175,111,50,175,64,68,36,222,1,125,34,106,32,107,160,216,33,221,46,17,168,95,246,59,220,35,140,157,123,128,66,220,239,138,25,219,233,62,197,208,233,24,170,243,134,145,254,34,75,52,154,48,138,195,253,
130,155,189,166,39,228,130,195,130,244,244,44,117,162,39,35,50,71,189,209,195,154,57,129,119,87,199,155,78,32,226,66,16,246,4,215,234,62,1,152,116,110,22,191,71,161,61,14,69,28,123,26,232,13,12,7,195,
221,37,68,152,233,153,48,47,66,138,169,47,6,220,161,249,133,220,84,187,159,64,8,34,166,111,70,107,204,155,62,68,236,167,72,216,19,88,164,246,23,2,100,45,204,222,60,245,90,195,45,68,144,108,199,233,71,
82,240,129,96,89,132,53,152,60,205,13,130,217,94,11,172,201,28,186,35,71,224,205,90,154,213,129,131,252,100,174,169,88,0,236,102,96,16,24,77,17,66,209,32,2,66,207,126,115,24,175,88,99,126,192,29,235,70,
35,220,167,227,185,19,19,253,69,28,136,186,131,217,41,210,113,165,64,161,123,250,208,119,2,238,153,201,16,226,192,77,77,138,81,0,130,196,68,195,253,205,210,39,229,57,241,163,34,68,135,105,110,248,11,181,
162,5,120,139,148,158,150,199,190,7,161,134,185,100,129,53,176,22,88,160,14,23,89,147,161,160,31,7,56,124,130,190,188,197,154,157,88,2,88,214,231,210,203,72,113,145,138,248,172,195,172,236,102,189,19,
73,194,189,247,217,182,128,84,88,1,20,61,14,206,10,192,122,71,1,251,46,18,151,36,96,7,231,78,176,222,117,15,209,92,34,20,136,104,174,38,102,19,111,114,111,244,192,152,58,130,225,132,137,192,220,144,63,
17,33,160,97,62,143,114,233,115,145,35,155,50,243,62,7,208,160,251,212,113,57,53,8,183,70,167,245,214,230,240,158,165,247,231,125,92,211,210,82,244,230,185,206,24,19,202,130,35,214,152,52,168,140,228,
102,99,29,152,71,176,4,232,128,153,195,129,198,10,161,16,251,41,179,148,178,138,82,88,172,236,24,239,129,246,83,236,138,224,41,129,228,125,122,173,2,58,185,195,162,97,160,76,1,148,44,31,154,155,211,204,
152,164,168,108,32,209,44,178,66,28,223,11,160,133,147,102,153,37,47,50,7,153,27,221,105,68,239,88,38,230,102,241,22,109,117,144,75,244,8,8,66,209,45,55,18,40,151,35,161,24,255,151,88,99,204,31,136,232,
180,41,207,16,17,184,27,65,74,215,89,49,32,216,243,224,56,65,4,119,56,105,7,103,189,64,40,68,192,197,215,132,101,132,209,187,79,5,129,203,155,29,40,146,98,61,210,239,1,224,233,15,5,7,165,194,105,46,195,
36,133,132,99,142,142,209,138,22,232,136,67,248,147,215,245,166,140,142,24,107,139,230,79,160,3,102,176,95,80,244,54,229,161,55,135,144,189,82,71,148,170,75,23,30,45,36,4,233,0,74,4,114,17,132,20,189,
211,188,65,208,82,97,197,147,67,107,142,111,150,61,91,37,128,93,119,218,10,23,216,133,98,13,177,174,239,228,238,26,9,143,104,41,144,4,251,27,209,177,212,113,48,71,49,39,147,131,53,14,198,5,129,211,3,60,
34,136,32,121,10,71,223,189,39,124,188,153,99,4,233,104,161,163,128,214,152,27,205,59,79,128,57,14,59,77,97,234,64,168,91,242,30,204,188,5,157,70,160,70,238,148,255,45,214,56,145,171,71,247,230,52,130,
73,199,188,116,105,18,182,232,248,7,172,186,107,21,220,83,201,62,53,219,87,78,41,173,181,171,177,63,239,233,139,192,192,152,9,70,161,55,137,60,189,33,9,39,68,224,128,97,83,30,26,16,9,204,12,248,164,194,
32,199,14,14,20,249,203,27,253,211,113,138,66,54,33,214,184,224,83,236,62,95,112,3,91,89,147,217,133,80,132,98,174,150,12,247,80,15,166,211,162,161,127,139,53,6,32,212,172,211,18,72,6,122,41,172,30,34,
32,2,152,64,6,216,217,33,118,221,35,92,89,243,126,44,95,160,21,79,250,104,228,114,138,25,184,140,63,254,163,196,40,139,215,170,216,174,11,190,120,229,122,171,128,219,167,3,94,70,221,54,195,107,133,186,
213,18,48,42,80,90,101,148,145,41,231,90,54,62,106,25,215,229,118,45,229,86,111,75,41,231,219,199,185,213,178,92,255,181,245,143,237,207,175,40,198,86,241,170,91,173,176,93,255,85,191,183,143,43,237,60,
198,40,165,48,102,2,218,160,214,109,251,88,160,214,219,149,51,203,109,241,229,179,1,94,74,20,128,243,40,231,143,121,251,71,101,248,135,229,25,116,84,65,9,149,161,208,24,0,26,250,114,112,166,241,227,219,
99,59,51,214,101,95,79,247,47,237,28,191,126,194,53,190,223,230,54,161,234,149,202,181,180,202,173,110,31,215,193,63,254,83,1,106,150,17,45,197,107,48,10,14,73,168,72,60,240,73,150,10,17,110,207,218,10,
68,6,14,49,49,143,126,66,180,140,56,161,102,186,147,42,95,128,99,10,181,222,82,207,110,116,2,29,226,250,211,32,28,67,208,58,185,145,196,248,84,4,76,102,212,210,127,139,91,42,211,56,164,218,149,131,207,
81,128,8,96,222,72,201,250,62,47,90,124,241,154,172,127,204,211,153,71,92,37,7,250,227,159,101,192,168,94,199,117,241,237,99,171,144,71,228,80,110,215,243,12,8,12,38,160,189,226,84,5,163,140,234,231,60,
31,8,49,40,49,174,131,235,178,109,231,168,12,239,165,220,206,156,75,9,110,149,177,213,241,253,231,240,143,69,1,124,251,251,26,126,94,70,173,215,64,44,117,148,114,59,51,110,215,60,238,232,24,118,89,170,
111,103,226,10,124,92,75,92,127,115,237,220,150,49,74,1,162,124,216,13,171,120,191,245,219,60,122,167,214,18,104,32,134,198,132,213,220,158,246,224,129,22,252,107,173,235,233,215,247,219,183,239,99,110,
57,91,31,40,52,170,87,216,42,181,142,135,111,213,43,91,15,141,55,62,23,51,220,221,195,17,132,53,166,71,27,77,110,156,117,240,100,24,45,166,233,18,0,14,142,140,54,159,183,78,35,154,100,100,60,193,125,158,
158,192,187,49,119,88,222,37,2,233,105,195,173,159,240,42,66,74,223,42,241,127,137,53,130,166,67,35,58,134,203,59,157,67,97,5,36,52,89,214,26,176,66,254,214,10,135,8,227,212,46,167,134,1,74,195,81,145,
156,44,113,205,45,15,207,129,208,155,152,182,2,96,62,93,12,159,194,20,147,33,107,14,35,63,153,203,241,151,10,118,212,76,155,81,169,88,101,168,230,120,228,10,235,77,247,8,192,20,206,234,171,239,79,51,21,
68,115,59,130,230,130,156,105,228,72,20,240,111,177,70,55,162,247,32,82,73,239,70,127,237,145,74,133,53,226,120,119,135,35,88,35,2,188,97,230,232,196,157,83,152,179,50,183,153,174,145,107,15,142,123,178,
246,158,96,127,11,82,169,241,99,12,102,164,186,239,244,198,97,219,135,18,86,154,121,160,145,204,193,39,231,56,120,71,111,244,222,12,167,227,141,152,218,133,240,30,46,58,39,156,116,148,240,110,239,23,69,
14,173,161,0,51,69,54,164,56,184,204,44,65,123,70,44,102,177,100,47,77,76,201,149,128,242,227,5,82,180,107,206,93,90,153,0,92,119,50,229,87,220,219,60,126,105,95,149,75,189,191,58,88,49,232,62,227,33,
211,7,255,212,1,13,4,79,167,31,70,28,168,19,128,233,153,255,3,118,96,154,242,39,132,195,113,164,80,16,157,126,120,188,194,250,76,175,202,246,242,36,117,236,137,70,253,254,90,173,172,150,112,209,243,127,
245,127,139,53,38,91,8,131,102,1,154,9,174,97,144,48,2,15,166,236,65,16,59,129,214,117,37,99,244,64,220,223,233,185,223,151,189,29,51,99,37,145,100,191,224,222,140,88,33,16,111,137,19,102,230,205,253,
112,93,133,148,189,78,11,47,191,131,67,221,250,77,5,155,144,34,210,83,161,160,19,4,244,116,56,56,201,147,32,32,189,17,159,150,47,85,183,231,57,247,66,161,215,116,3,193,177,176,175,34,38,71,144,48,112,
204,105,41,211,221,53,225,67,116,137,108,13,136,108,117,63,237,136,6,59,14,187,89,188,3,216,10,190,38,180,214,119,192,46,238,188,211,187,145,216,22,161,55,205,53,196,204,94,72,204,28,234,241,112,246,70,
116,7,240,151,107,144,79,19,9,11,14,47,29,180,150,132,139,44,104,162,37,8,130,207,117,38,239,199,238,107,100,174,91,16,207,153,37,70,196,28,134,166,225,12,76,251,120,186,127,122,130,3,147,26,6,30,17,145,
239,78,72,231,251,172,217,58,97,172,96,206,234,112,89,221,156,221,47,54,175,192,206,116,219,221,146,148,36,18,25,136,183,152,107,8,137,40,238,51,98,47,252,223,56,42,206,252,102,178,43,56,70,244,20,8,61,
250,19,250,120,199,213,122,115,143,116,182,132,31,44,63,203,106,152,225,68,104,223,149,178,40,31,204,53,208,1,55,193,132,163,11,194,228,242,140,245,39,123,180,102,201,181,140,160,75,57,135,252,159,80,
160,85,113,108,167,13,96,95,181,154,239,230,236,216,106,43,246,142,77,218,222,33,89,138,90,228,154,229,194,253,241,207,49,85,102,0,45,181,110,27,31,149,81,40,11,175,3,76,0,184,102,202,44,80,189,78,237,
190,12,24,165,140,161,81,6,10,77,93,6,168,87,150,43,31,75,173,91,21,5,130,90,202,64,141,74,165,194,143,234,21,106,25,162,140,194,208,80,70,201,242,30,178,33,8,84,96,140,66,12,81,74,89,110,21,196,50,54,
187,134,202,178,213,18,181,114,45,240,193,114,45,67,81,104,61,74,33,64,101,168,140,50,24,223,249,245,40,3,84,82,89,123,252,218,169,254,237,171,127,219,249,81,31,94,171,198,250,149,235,156,96,197,235,118,
126,110,248,3,175,227,13,199,159,222,16,72,133,192,61,87,241,201,176,178,68,40,94,124,56,255,53,81,65,132,166,159,45,152,168,170,8,5,56,222,21,4,225,154,8,195,58,155,241,78,242,177,128,136,245,147,220,
158,37,16,17,13,162,201,33,34,48,57,17,96,10,165,243,196,149,247,88,152,37,14,182,214,81,107,211,114,141,67,63,59,41,64,119,194,133,86,237,0,59,151,247,21,115,76,153,209,118,204,71,32,107,201,152,4,208,
229,111,152,31,54,163,8,48,82,237,76,103,9,255,173,88,66,201,146,104,87,8,77,186,11,44,133,117,54,158,9,141,78,55,46,160,100,252,192,83,237,1,210,51,62,131,13,226,20,207,203,47,68,40,27,139,54,105,62,
172,145,132,153,172,213,67,129,201,20,132,161,9,27,111,200,122,48,243,227,35,16,173,131,136,228,63,123,96,40,114,127,153,57,187,237,142,177,18,236,171,66,241,92,201,54,135,146,97,129,8,222,142,121,155,
125,10,239,253,155,186,249,42,41,14,95,101,135,41,9,5,204,233,37,236,12,34,90,135,25,32,137,4,0,115,69,178,244,147,187,11,34,8,8,66,40,155,12,34,117,74,159,161,56,203,174,195,143,244,166,153,3,224,168,
75,208,137,166,192,58,40,20,202,97,1,153,60,26,128,96,205,235,32,178,31,95,177,213,113,115,216,239,192,41,20,210,110,228,210,211,27,141,35,97,92,253,127,8,220,45,20,194,243,138,128,255,159,162,104,124,
134,86,74,178,9,173,56,240,42,18,90,224,102,225,54,107,188,254,124,42,173,119,130,102,121,90,178,66,161,116,147,199,81,67,79,112,77,252,56,72,173,183,68,179,78,214,214,212,250,90,7,133,203,231,218,16,
164,50,10,247,29,130,149,117,231,254,142,185,237,164,87,219,200,187,9,34,215,11,71,179,123,207,160,66,126,124,35,241,168,165,234,113,112,34,142,104,240,239,37,242,251,121,18,161,31,166,122,40,127,41,152,
176,82,62,56,240,89,180,79,64,60,192,45,209,35,2,186,56,61,221,78,123,226,24,130,85,16,71,117,183,156,140,128,16,115,203,110,74,61,64,17,157,32,162,103,229,48,98,54,3,68,70,194,132,210,159,165,221,12,
99,93,33,213,211,79,154,95,64,70,211,1,235,56,120,218,202,83,117,112,1,58,20,216,79,229,191,127,195,1,47,158,14,214,185,178,145,163,11,66,19,42,102,137,226,22,244,208,39,148,3,3,69,88,204,85,207,124,131,
231,83,16,136,29,86,79,203,4,145,166,145,136,236,4,122,168,245,188,160,23,133,20,2,17,17,17,233,150,212,177,126,136,29,181,61,4,28,0,118,216,223,89,125,221,253,223,231,233,14,52,35,60,140,158,177,9,135,
63,254,89,202,40,203,216,174,231,118,253,96,219,62,159,232,236,219,127,71,47,13,188,214,141,203,131,245,71,29,71,109,175,131,1,168,148,82,98,128,215,245,1,84,55,124,187,182,60,219,120,0,228,189,103,42,
129,87,13,10,247,159,250,57,84,126,230,83,149,129,202,253,49,6,12,30,233,130,67,113,13,64,133,64,101,0,99,12,202,199,153,51,133,82,66,132,24,47,120,143,194,40,20,6,26,5,24,148,69,99,148,1,219,2,224,84,
223,108,221,183,175,84,255,60,207,245,49,21,135,109,171,21,182,243,184,150,229,118,253,216,234,155,2,5,78,64,152,25,230,238,32,14,153,39,126,43,10,232,96,236,122,6,177,0,186,166,10,66,68,0,244,23,98,155,
133,53,38,185,204,102,20,72,115,123,202,201,3,8,4,8,2,14,245,81,98,138,223,167,123,38,208,179,165,105,140,69,68,194,42,185,58,207,138,57,146,89,91,179,61,77,200,36,29,238,62,121,214,179,14,239,184,59,
230,151,89,47,19,12,163,155,191,133,34,144,153,135,185,59,105,199,120,178,136,223,232,82,0,79,25,147,121,27,126,64,53,121,133,14,72,37,230,3,135,83,199,127,75,6,201,211,192,34,200,115,221,255,47,54,28,
160,149,169,70,97,204,41,235,55,88,145,108,41,196,243,125,193,243,39,128,8,82,251,203,197,186,223,35,111,159,5,28,214,203,14,73,131,47,118,50,67,90,216,238,207,24,83,0,33,123,211,252,96,96,233,125,48,
243,116,86,56,160,99,187,152,66,238,220,53,71,228,110,118,132,230,14,217,241,116,86,36,130,8,7,35,51,56,124,206,27,242,133,136,132,237,81,25,69,38,225,113,204,52,118,62,155,91,234,226,149,245,174,227,
111,40,20,36,106,193,228,103,71,27,57,85,131,3,66,251,73,51,20,182,2,190,239,107,34,24,152,175,153,177,140,31,255,67,170,80,110,199,144,222,34,196,241,44,163,109,128,146,219,175,48,173,115,212,20,160,
169,49,33,35,111,223,72,201,24,64,136,152,108,85,2,20,137,221,6,134,117,183,227,52,95,121,250,90,148,213,239,204,164,13,246,80,228,173,85,130,236,104,174,212,132,98,15,38,98,197,28,127,0,135,162,46,0,
229,16,242,65,78,203,9,118,200,195,215,239,194,65,236,110,152,179,59,235,172,182,251,62,215,53,65,196,204,90,16,71,241,55,169,33,76,194,73,196,2,227,210,132,236,221,20,233,180,114,122,96,189,37,157,137,
12,49,160,112,203,125,76,28,36,243,156,77,32,29,94,69,247,102,185,196,210,11,253,214,30,211,166,127,173,246,30,235,1,42,244,73,160,135,18,26,7,24,94,37,107,39,183,122,253,2,192,16,120,71,224,138,61,2,
78,201,92,2,195,49,48,127,79,15,70,135,132,83,62,152,106,249,68,132,57,230,63,254,99,228,161,48,65,69,75,205,35,22,202,151,101,212,225,108,231,145,59,28,174,237,124,171,106,54,0,184,93,207,183,15,195,
173,140,202,163,94,3,24,84,252,26,191,190,199,16,49,196,173,142,81,107,202,195,237,242,213,235,86,43,126,14,216,42,42,131,241,24,40,110,127,69,41,101,136,49,30,220,42,124,47,148,208,64,37,120,149,132,
213,237,26,204,112,74,26,220,26,99,20,98,194,138,49,74,9,197,145,178,234,54,112,43,192,168,229,215,67,227,215,23,194,235,118,165,12,166,216,175,53,55,111,148,95,95,215,191,205,183,74,173,94,171,87,182,
234,181,146,63,0,108,111,83,25,9,128,198,220,228,23,191,5,85,129,150,183,117,204,98,233,224,71,225,206,1,118,192,88,79,1,129,11,250,250,252,86,188,251,100,155,1,79,244,146,146,13,205,76,98,145,185,37,
65,40,224,254,108,22,80,36,6,5,204,97,28,127,32,97,21,47,28,152,200,59,217,206,241,228,80,118,109,230,63,248,147,59,229,25,190,39,246,73,92,73,127,110,76,31,188,39,87,246,254,150,116,47,248,212,126,39,
245,3,239,222,193,65,189,101,92,17,4,206,165,229,29,228,62,41,11,128,85,4,129,16,189,69,196,78,42,119,78,51,236,245,178,104,196,124,237,222,187,38,13,41,227,205,207,209,236,47,134,5,208,130,214,208,167,
165,225,19,224,20,58,96,117,208,43,120,90,41,74,142,184,10,165,244,157,179,157,130,250,226,73,235,129,155,153,99,222,49,245,148,120,0,14,115,179,228,207,149,173,0,0,26,104,73,68,65,84,192,27,72,49,23,
206,12,143,67,95,144,176,231,38,49,232,145,190,228,38,186,239,29,223,233,145,156,16,112,73,167,112,223,33,20,17,207,3,151,225,179,103,59,192,20,70,206,58,1,202,106,168,177,134,34,2,9,221,39,196,14,134,
165,148,130,141,87,46,208,65,119,10,136,132,213,241,32,241,146,201,113,156,180,227,115,135,78,16,56,107,187,187,175,118,73,120,237,207,73,216,252,27,168,29,36,229,24,71,208,134,183,217,229,209,153,29,
235,17,185,82,230,50,68,100,192,215,92,150,27,228,100,233,194,48,46,220,51,172,22,110,166,185,218,59,128,187,33,215,177,171,100,231,183,240,97,204,30,57,121,132,105,159,209,74,14,118,63,159,11,194,28,
14,228,124,142,244,249,126,66,139,103,117,18,79,113,235,193,100,30,147,91,8,97,190,219,187,177,231,173,126,36,238,219,209,174,77,27,52,197,2,6,152,245,105,40,255,241,79,74,148,193,115,235,89,89,252,28,
80,198,128,245,107,218,6,99,48,174,3,223,234,246,231,242,119,229,122,251,88,56,246,182,61,198,163,250,21,202,175,191,74,201,247,102,217,46,143,107,212,1,191,30,85,101,168,44,21,214,159,215,115,48,198,
64,191,30,26,229,254,133,243,64,225,182,126,165,252,250,18,99,125,140,207,176,26,192,183,239,101,104,92,7,26,147,177,167,109,21,249,33,20,164,117,36,98,160,128,1,213,23,160,140,2,227,154,196,19,37,160,
166,172,121,248,118,61,47,100,86,195,117,148,208,128,234,182,125,123,120,189,158,39,252,171,111,117,202,139,146,152,21,252,206,10,14,83,126,7,140,21,220,21,14,70,110,207,72,203,30,0,211,138,211,185,71,
188,103,0,245,201,106,86,222,5,128,78,16,145,60,92,251,171,171,93,112,63,181,214,224,238,248,65,43,83,151,91,1,142,129,68,28,168,62,41,46,219,80,86,57,40,67,34,144,98,62,233,154,117,66,136,249,63,164,
138,158,211,225,224,211,130,144,187,27,182,11,34,60,132,79,198,150,10,14,241,198,111,37,29,128,191,69,21,148,32,123,82,52,64,188,146,205,26,116,181,216,113,219,211,121,113,154,131,122,239,253,96,78,57,
43,77,101,232,104,4,194,247,134,117,116,50,235,187,194,9,41,242,253,131,237,206,25,190,202,243,171,39,246,197,179,78,204,159,231,175,9,185,60,146,63,72,54,102,62,195,134,128,58,7,47,108,201,95,21,134,
99,30,78,178,30,14,229,238,237,128,155,0,152,9,253,249,1,192,9,86,46,211,247,254,82,168,251,12,99,171,239,180,251,5,64,50,90,136,152,232,96,1,70,200,81,78,39,226,232,117,205,46,26,221,123,23,14,171,55,
71,61,209,39,237,235,53,167,122,128,4,56,112,228,133,88,1,135,221,151,31,245,169,250,243,83,4,152,178,194,193,202,223,49,82,110,130,86,18,222,153,127,237,9,52,239,25,196,50,144,16,161,55,29,43,47,135,
68,158,246,234,207,187,59,59,239,1,184,77,186,4,226,174,153,30,31,194,222,223,89,233,45,132,249,191,133,150,165,176,76,160,252,44,177,246,4,131,57,180,214,232,45,118,235,180,251,51,49,88,172,204,74,247,
207,205,113,159,112,226,88,81,197,36,167,3,116,175,34,181,134,199,4,68,68,68,76,6,238,158,178,82,145,154,211,33,119,83,196,24,56,158,74,166,227,16,145,139,248,199,127,104,0,131,49,170,70,230,217,108,223,
254,117,184,169,234,216,12,184,14,182,111,127,215,202,86,215,31,21,96,249,169,113,29,117,253,169,128,90,253,171,103,216,168,254,56,159,25,243,222,174,159,131,49,128,234,181,68,249,245,147,1,190,85,63,
223,182,5,24,58,179,108,216,184,81,253,219,191,236,27,255,243,191,30,37,224,193,175,199,131,201,232,143,187,211,82,141,126,148,50,72,246,94,128,228,247,94,147,183,127,2,152,6,162,245,37,175,7,27,175,38,
234,182,85,183,154,220,104,70,246,110,215,193,24,212,205,234,0,188,186,109,70,101,99,195,94,254,189,241,150,130,22,220,9,129,161,188,166,110,198,181,232,76,94,181,3,60,179,92,118,133,135,107,87,35,79,
154,191,180,12,78,24,132,235,216,190,64,34,64,7,165,70,224,73,217,221,65,106,119,212,109,117,51,250,9,63,189,239,51,186,195,62,127,94,68,165,233,47,141,64,17,36,10,70,182,159,200,30,60,153,203,44,22,56,
253,181,185,209,0,183,195,5,243,252,218,45,114,142,150,230,108,246,148,105,66,191,121,169,222,32,164,144,204,252,232,219,49,115,127,233,163,159,194,237,175,87,3,60,149,175,221,78,110,254,158,60,2,34,212,
239,206,161,132,42,86,20,202,93,100,73,58,135,54,31,246,222,162,37,131,184,55,46,237,194,20,130,199,175,23,172,8,102,6,235,193,149,18,46,154,109,2,89,61,63,105,214,197,8,66,199,192,157,99,251,11,28,201,
246,121,248,226,164,229,22,32,199,156,152,106,221,103,104,253,241,79,24,131,194,152,106,211,40,101,193,235,102,206,86,61,49,120,84,168,108,21,55,72,119,49,112,93,126,125,213,253,123,185,109,31,198,102,
155,225,103,134,110,181,240,229,220,190,61,150,7,104,220,234,67,20,150,235,216,182,74,249,243,175,229,154,111,220,206,183,90,171,62,182,191,183,94,198,227,219,223,127,243,215,175,135,255,53,64,63,1,30,
159,97,165,1,20,226,32,180,252,88,198,24,135,134,136,79,131,30,160,196,40,4,3,148,41,165,211,106,134,178,228,205,35,212,73,132,27,211,112,6,180,232,124,230,35,111,7,210,128,122,216,219,94,201,253,134,
228,8,28,232,57,150,3,253,166,223,223,231,7,0,103,63,100,162,103,122,66,79,86,111,56,1,173,139,214,224,61,125,95,142,59,113,15,65,55,60,246,166,128,14,110,205,228,96,121,154,171,191,115,185,180,119,12,
145,103,135,174,126,8,171,67,10,235,5,171,44,137,92,199,23,214,142,234,179,8,92,179,202,140,73,72,56,116,132,51,241,42,137,51,249,67,115,90,28,104,216,230,63,158,147,215,91,142,46,27,79,17,31,207,222,
156,167,111,242,51,233,38,219,210,253,136,224,229,49,68,107,178,1,139,59,102,25,15,116,238,214,47,118,103,159,1,92,155,156,38,186,95,48,129,7,172,4,116,227,125,191,176,122,220,83,16,242,14,10,9,214,217,
227,111,112,202,33,252,254,241,55,203,81,208,114,248,130,131,244,157,185,233,133,213,94,64,152,105,144,152,119,123,146,169,235,32,64,59,90,109,111,7,237,230,46,148,132,54,238,233,110,117,11,158,196,240,
82,178,86,137,53,222,69,211,189,181,75,191,96,83,31,15,173,14,235,106,247,59,168,251,73,113,98,7,187,52,26,171,211,250,92,70,156,8,204,39,36,238,244,139,222,57,193,59,239,64,184,25,177,18,169,20,19,40,
55,5,206,193,232,5,170,227,47,66,207,15,1,205,48,75,8,31,75,109,121,198,6,138,83,174,156,115,192,202,28,245,166,214,8,186,187,12,166,36,56,138,219,27,150,154,94,196,167,85,192,220,177,25,9,14,210,31,243,
116,2,112,138,224,29,139,232,141,110,239,241,14,14,193,186,139,83,135,247,19,167,19,112,191,4,51,243,117,183,213,214,221,176,136,21,235,141,19,228,46,2,119,28,189,211,118,210,54,231,2,235,106,160,99,151,
110,150,157,207,76,140,131,10,63,125,214,252,135,192,95,24,17,79,122,210,113,174,71,36,100,83,196,62,221,162,121,108,141,121,111,102,33,94,140,39,139,241,230,104,246,34,205,158,166,191,245,147,32,152,
201,226,24,244,204,161,117,195,81,88,94,76,186,98,171,251,204,122,220,239,157,149,123,172,236,59,113,98,226,198,206,14,142,252,100,142,89,24,152,194,60,183,229,176,118,238,189,91,158,224,195,59,190,38,
235,154,42,7,4,255,93,218,241,153,173,0,49,53,223,224,247,98,224,184,53,34,60,83,72,34,8,5,100,236,207,115,181,59,205,186,50,39,182,29,32,214,1,116,231,31,255,155,102,100,142,153,146,32,129,84,115,205,
205,59,129,34,87,84,180,78,100,190,125,214,17,132,31,137,247,89,91,129,32,109,158,117,119,46,59,10,221,247,117,119,12,133,95,118,69,230,246,155,238,59,164,6,2,204,157,6,207,178,102,162,203,243,227,123,
143,39,110,41,88,79,247,125,210,208,235,181,57,208,64,243,116,35,17,185,178,150,147,18,225,157,214,155,241,156,215,196,194,249,81,36,99,123,14,198,159,237,255,241,127,150,173,186,221,42,94,71,122,107,
210,85,177,109,70,213,25,134,24,195,43,26,37,182,101,240,128,173,226,149,186,62,70,225,102,213,191,122,197,55,182,222,22,202,16,49,198,250,115,160,47,183,190,252,168,148,49,30,60,244,65,229,86,235,131,
161,243,168,212,186,254,235,107,230,178,45,223,254,182,235,249,246,231,227,250,235,199,86,57,174,27,31,57,214,107,254,125,212,219,117,28,186,248,128,239,124,25,176,213,235,242,10,162,103,42,156,24,130,
243,240,138,136,244,162,155,87,167,170,196,237,122,134,165,76,239,205,0,168,121,127,217,240,69,3,198,184,209,99,12,14,223,123,117,75,205,69,127,252,167,27,149,235,160,162,18,140,212,84,220,54,42,138,81,
130,18,26,21,133,24,21,84,202,216,42,219,183,7,223,135,104,151,31,117,171,219,86,191,253,109,87,254,124,40,68,104,232,167,134,136,111,255,170,57,193,245,161,251,215,234,245,219,247,91,133,49,208,208,120,
204,176,249,173,62,190,253,248,248,176,135,127,252,109,21,120,172,203,242,128,132,90,29,235,242,88,31,176,126,29,135,138,167,161,81,104,21,42,101,248,19,88,133,0,74,204,52,67,175,37,160,122,117,115,106,
173,220,22,239,109,201,105,162,12,140,224,155,185,177,109,245,48,139,234,140,239,111,83,245,220,170,218,86,97,188,61,117,10,4,2,151,3,70,55,41,80,76,171,103,22,39,38,91,221,229,33,176,253,226,230,102,
190,95,196,125,79,84,142,53,8,197,157,125,198,93,253,68,236,142,249,222,76,176,162,16,2,3,225,221,121,167,95,36,51,230,217,249,243,30,223,84,233,246,253,48,127,158,151,176,16,147,206,36,252,96,46,16,115,
18,16,224,32,196,75,170,57,221,59,57,164,151,42,164,164,229,231,119,62,85,122,38,131,55,55,111,102,8,254,241,15,155,44,72,173,147,196,236,7,181,43,192,123,235,145,79,227,104,60,235,40,38,139,72,162,86,
184,41,116,63,17,243,8,189,208,157,29,252,242,62,93,100,201,219,116,63,44,64,64,225,112,121,135,121,178,15,78,143,217,181,158,181,142,202,207,33,132,207,225,232,254,20,210,191,213,158,85,21,201,177,56,
88,86,179,217,120,194,246,85,114,10,60,89,212,193,228,220,192,103,144,244,237,229,198,179,8,44,166,130,54,65,33,58,105,102,38,79,7,158,18,59,148,189,112,1,71,228,93,74,123,4,233,27,69,236,59,228,149,128,
109,118,130,102,134,162,112,64,141,11,246,110,16,214,90,56,234,61,86,66,177,206,100,61,189,244,59,82,201,225,243,44,239,167,207,152,53,123,120,138,202,227,171,103,137,158,58,146,3,159,55,32,227,184,211,
68,138,41,192,240,3,202,228,92,131,55,135,84,102,67,126,36,45,120,194,61,158,245,114,181,1,92,13,183,116,249,198,154,130,100,119,204,231,185,100,98,205,20,52,136,59,43,176,247,184,204,160,210,14,196,76,
178,13,153,220,27,254,110,116,25,14,221,13,162,9,184,107,103,247,76,54,123,154,87,217,106,130,192,38,248,79,192,75,165,122,130,231,0,8,128,57,221,178,146,59,46,36,12,125,58,148,57,197,161,201,26,152,91,
195,241,195,251,224,9,176,16,248,155,205,244,6,136,100,33,221,249,132,197,185,176,147,154,49,184,131,241,190,154,4,123,92,112,200,75,74,47,179,145,253,148,179,146,82,171,39,62,229,38,173,28,23,240,172,
161,187,153,97,230,68,120,207,96,155,240,14,119,165,214,176,238,251,61,86,118,50,105,99,37,17,235,14,136,142,131,238,177,206,198,35,96,37,124,194,83,7,4,69,63,38,160,110,205,166,190,240,9,61,143,108,153,
150,96,203,252,59,51,176,249,31,1,225,240,230,78,50,90,199,210,81,148,106,88,218,36,34,78,49,17,203,13,232,57,241,19,209,146,247,27,240,110,92,142,56,3,154,176,138,204,224,156,211,132,156,241,19,75,78,
207,107,139,204,133,5,186,35,26,180,118,154,111,216,190,174,199,29,99,201,225,211,79,122,138,68,160,180,149,102,226,138,16,236,208,21,136,32,28,158,119,36,199,132,87,200,164,196,186,56,6,130,131,25,102,
178,204,139,177,35,187,29,156,35,102,232,152,120,195,200,69,154,205,210,8,20,97,2,159,70,80,150,79,38,247,188,17,250,174,93,224,6,246,62,223,70,65,206,37,185,249,250,244,227,177,179,51,195,204,174,153,
56,227,38,57,33,201,98,39,186,69,199,218,78,219,29,208,190,71,202,67,124,182,165,164,180,32,194,153,152,150,40,36,152,99,200,58,134,66,246,28,143,12,133,62,87,156,197,205,102,104,201,129,105,185,76,94,
149,176,155,95,55,251,227,63,129,161,243,24,53,115,45,70,173,190,125,108,219,181,113,61,135,134,6,168,93,9,52,94,26,77,221,42,181,62,6,165,140,235,168,94,183,243,84,82,134,2,133,95,227,251,88,255,235,
199,246,117,93,30,235,242,224,169,105,254,168,192,183,159,252,168,235,143,234,198,109,217,76,113,91,84,6,148,33,206,55,234,205,214,191,198,250,40,99,125,76,205,234,219,143,202,3,198,175,239,163,4,136,
95,143,188,39,55,31,251,50,242,64,50,95,224,246,177,140,91,133,82,142,188,201,81,10,67,173,150,16,83,61,123,150,10,91,173,126,37,247,175,186,121,173,84,175,27,219,86,221,182,90,61,15,88,167,254,227,63,
97,146,142,166,72,13,63,180,135,151,56,214,212,35,48,212,204,109,18,247,92,229,117,135,227,117,158,47,42,142,247,39,146,237,249,143,117,159,41,228,106,6,130,84,26,20,130,192,83,150,172,251,154,27,237,
214,249,115,40,52,129,136,151,246,177,238,10,192,47,135,6,1,255,86,119,34,84,76,186,200,125,214,159,134,10,254,137,102,152,172,153,105,145,249,36,76,28,227,237,168,34,66,147,64,15,237,1,66,32,233,69,252,
0,166,206,145,245,22,17,136,247,148,142,47,12,87,240,188,108,0,38,241,237,176,227,235,186,179,27,41,52,37,145,57,203,208,32,2,44,32,96,103,127,159,20,188,178,195,154,158,143,156,115,178,175,217,52,240,
100,3,179,164,48,211,252,20,249,71,68,38,19,0,135,218,147,165,31,106,68,178,169,148,220,230,201,95,152,57,91,102,51,215,225,233,114,180,228,252,19,84,9,192,0,26,169,23,100,59,233,78,55,51,71,40,130,203,
238,74,230,43,86,73,0,10,226,5,175,29,124,117,112,219,119,208,186,114,58,165,10,158,203,30,114,19,207,55,65,96,145,105,234,251,186,174,235,126,28,68,231,243,210,184,29,196,10,107,76,25,198,154,192,7,122,
30,92,58,193,255,41,167,52,230,112,208,19,148,64,224,102,13,63,26,202,255,82,28,206,15,249,245,31,63,211,254,25,165,140,35,7,236,89,6,162,20,66,231,50,96,48,84,6,195,249,232,3,223,170,91,218,162,250,87,
173,165,124,97,136,224,49,74,48,224,215,99,249,30,211,22,102,125,176,126,253,97,7,175,24,223,255,181,157,219,159,127,141,81,248,245,19,191,150,18,91,45,64,41,67,12,214,229,39,126,29,143,7,160,241,120,
60,30,60,38,215,187,126,9,20,232,86,25,60,224,65,182,186,109,127,87,74,251,168,224,203,182,125,44,37,38,99,170,5,200,219,141,184,206,139,27,202,11,110,100,3,213,205,45,79,39,158,91,214,168,94,189,110,
244,81,125,171,91,173,252,241,143,109,219,54,243,58,110,213,23,13,52,99,105,128,134,166,223,255,246,81,85,6,130,50,168,117,91,188,214,173,86,24,12,20,117,125,20,218,159,143,241,235,1,74,43,245,75,249,
153,176,95,31,240,240,235,207,99,115,218,186,60,124,169,245,214,127,150,219,21,190,148,178,140,66,57,143,49,198,40,161,24,240,253,39,124,251,82,6,176,254,156,162,193,191,237,235,3,126,125,25,106,219,53,
182,170,219,39,54,189,126,173,21,113,94,128,202,245,188,48,52,124,171,128,159,163,132,215,50,134,202,203,25,225,137,36,94,33,127,106,218,57,135,12,243,173,38,0,235,208,216,242,210,220,55,179,142,101,106,
123,167,193,103,54,253,44,221,136,64,228,111,129,125,166,213,213,119,181,232,187,179,39,103,16,16,49,37,117,238,170,13,252,14,56,188,239,186,136,244,89,70,68,68,136,104,132,96,213,108,51,152,30,67,135,
116,145,130,189,179,227,236,172,97,29,46,107,187,164,62,177,206,94,68,28,156,44,41,85,102,226,72,30,82,28,84,104,14,161,41,163,32,103,242,116,116,30,211,74,239,233,60,96,56,115,21,252,13,152,199,52,43,
133,72,204,218,40,132,136,144,146,139,8,2,214,230,76,230,224,249,86,236,23,232,220,101,18,184,238,220,231,206,146,128,217,187,175,216,14,126,113,183,181,189,135,247,214,64,142,146,127,24,68,166,148,204,
83,211,148,76,230,8,234,6,57,31,91,21,104,141,88,247,253,178,39,207,223,5,24,107,104,141,61,251,138,92,47,8,77,185,110,128,128,84,86,241,150,147,77,38,236,83,182,219,12,242,224,76,165,212,104,184,203,
193,48,251,227,159,220,174,103,218,181,140,210,236,51,5,142,50,74,106,62,233,226,26,133,118,29,124,95,234,184,22,8,188,151,242,235,49,6,252,184,198,175,199,163,140,219,226,231,165,124,225,231,119,6,107,
158,162,154,37,111,142,118,123,108,230,95,107,165,142,111,95,169,163,138,193,40,45,99,120,99,20,226,241,0,149,49,126,63,2,29,64,231,177,126,255,194,237,239,143,175,240,128,135,215,229,145,209,69,175,176,
124,143,135,134,134,219,195,107,201,239,80,235,121,229,27,101,48,40,4,149,58,74,38,102,121,5,223,182,154,161,67,124,163,86,72,87,225,102,158,215,96,214,90,71,221,12,223,234,27,121,172,64,4,153,37,39,144,
166,104,141,72,228,152,195,206,221,145,14,121,154,93,15,114,231,58,22,236,16,110,158,135,252,42,2,237,47,147,34,119,59,190,99,238,134,225,142,251,238,190,106,5,68,227,233,41,200,1,68,104,13,196,65,26,
89,141,96,143,136,222,59,176,34,224,29,86,98,197,180,114,10,129,7,184,119,69,202,176,195,79,7,36,6,231,7,5,207,182,13,166,98,149,86,14,120,226,88,82,166,25,142,210,92,228,13,53,29,52,30,46,133,20,1,145,
18,60,146,4,129,73,21,19,0,253,213,213,28,214,81,140,201,222,100,48,77,193,119,88,241,195,118,112,187,172,23,243,110,176,71,8,50,235,44,2,161,169,66,178,67,104,242,15,1,10,133,86,96,141,8,96,23,171,241,
186,210,121,87,112,39,186,176,222,27,242,233,32,56,206,130,56,52,211,192,1,33,99,170,174,105,8,154,155,57,51,100,63,39,100,62,59,15,75,182,254,6,22,241,73,131,59,240,97,142,253,88,152,249,158,191,168,
127,206,10,159,169,11,194,192,204,225,80,191,192,119,156,213,205,216,221,214,53,115,50,58,239,251,201,13,63,221,179,31,107,16,13,165,234,155,139,148,19,3,124,61,44,110,197,46,61,215,165,197,206,58,13,
236,117,13,98,213,14,113,151,238,7,132,100,58,108,12,103,98,173,48,68,203,4,91,127,34,216,97,14,186,59,51,115,100,250,104,154,50,88,231,1,127,252,115,0,149,1,106,181,222,174,203,192,231,145,63,101,12,
17,5,70,170,50,195,235,118,29,94,69,91,80,204,140,206,10,104,204,220,34,168,94,183,109,41,12,13,220,160,250,215,220,98,239,95,191,15,234,250,240,143,205,214,127,245,49,234,120,176,62,214,7,212,95,95,198,
181,16,90,70,238,221,87,57,212,130,245,235,99,125,0,165,220,255,235,177,254,28,215,228,132,26,91,197,191,2,15,240,175,143,245,225,95,127,125,47,101,44,63,199,143,101,142,170,20,6,229,118,166,140,43,37,
16,81,194,43,37,174,231,27,213,109,219,106,213,109,3,163,230,222,128,90,83,167,98,131,164,132,30,27,198,86,43,232,143,255,149,218,16,94,71,133,58,128,60,133,64,37,84,152,89,80,89,170,247,96,187,182,243,
121,136,18,10,13,80,158,250,93,230,165,218,126,249,97,219,149,240,133,245,43,185,101,177,122,133,235,24,235,131,101,249,187,47,60,106,54,169,159,254,85,101,240,24,130,12,34,13,64,68,157,218,213,247,193,
3,224,215,151,159,223,127,189,95,71,25,12,214,239,140,202,250,53,179,192,30,21,30,84,125,105,203,240,175,94,235,250,163,162,129,128,193,184,2,99,20,134,162,20,70,133,82,98,212,58,131,246,101,217,250,226,
219,182,85,55,175,94,73,51,186,214,109,219,108,171,117,164,211,192,43,71,126,22,201,55,252,96,64,2,152,84,126,176,41,96,158,232,213,227,174,39,153,40,114,131,245,211,74,219,47,185,173,216,121,79,162,93,
87,55,156,112,78,174,61,47,225,98,149,139,117,50,63,77,117,65,61,227,21,1,98,119,120,178,36,246,32,246,30,10,201,217,91,172,248,238,59,56,71,50,89,180,174,116,182,158,236,147,187,74,4,145,249,33,79,246,
27,112,184,173,136,231,126,145,126,120,223,147,91,25,207,244,24,205,247,254,7,115,167,115,194,108,50,160,79,173,2,32,8,209,44,242,161,142,71,145,15,19,110,199,88,246,20,108,233,113,76,57,226,153,160,31,
118,40,141,110,45,69,104,118,188,107,102,108,244,23,47,20,144,23,75,28,62,16,80,16,24,98,221,87,243,126,223,49,88,123,184,5,88,160,152,185,195,10,136,215,239,233,203,210,111,227,60,196,71,144,131,245,
231,152,179,79,115,115,195,243,248,118,3,120,11,12,57,78,76,169,52,139,56,208,42,18,86,113,180,238,207,21,2,116,244,45,15,92,2,164,136,59,238,248,37,129,111,176,227,230,172,126,202,11,25,86,155,34,69,
185,16,128,205,168,89,68,254,16,135,89,252,156,22,1,172,171,175,193,206,9,139,19,235,138,239,113,8,231,9,231,192,2,73,130,163,133,8,17,41,71,5,9,254,228,219,68,58,24,14,197,129,35,37,198,65,29,12,75,236,
112,120,115,131,152,107,151,210,200,149,134,186,64,72,199,116,196,236,120,30,202,69,206,55,226,128,157,251,132,43,232,164,110,102,187,155,195,5,7,186,95,200,77,146,59,235,46,163,229,68,68,76,25,23,249,
75,132,210,191,147,8,59,251,98,77,96,236,187,157,156,149,192,105,126,122,39,207,175,0,160,169,5,159,87,96,106,49,249,48,5,95,142,214,72,160,36,30,25,184,77,170,154,11,51,225,225,62,5,169,220,224,13,92,
234,29,33,50,87,210,26,61,20,57,212,136,236,226,211,208,173,63,191,126,177,1,2,51,148,177,25,111,16,126,44,221,238,118,113,107,246,78,6,75,89,33,68,127,66,22,122,67,135,85,5,68,134,203,4,49,185,86,159,
60,49,88,215,99,111,82,199,44,112,29,141,174,235,76,154,8,148,230,82,128,230,80,21,118,224,39,136,167,110,104,50,124,154,62,9,178,252,151,48,89,30,205,159,175,119,135,63,254,81,175,109,185,45,12,6,3,174,
67,101,161,140,116,149,140,92,205,40,20,10,3,84,74,25,211,153,91,41,99,128,34,205,154,138,206,140,43,165,68,189,182,69,11,155,213,13,243,141,186,252,207,229,239,122,45,55,99,125,172,143,199,242,125,140,
107,171,133,251,79,70,1,56,183,51,81,202,60,109,199,23,6,48,6,232,167,6,26,35,71,140,198,227,161,120,84,128,91,5,157,207,109,171,94,53,30,60,126,156,207,11,224,85,49,202,121,148,1,12,149,50,252,154,13,
224,149,82,40,161,35,63,98,171,183,202,182,29,49,235,138,103,38,135,13,175,183,173,150,145,123,90,184,93,163,234,246,199,127,50,170,242,88,72,13,20,140,219,181,100,186,64,246,198,40,133,40,137,4,49,110,
203,147,75,221,22,13,20,207,163,144,198,24,154,134,216,194,72,149,139,186,93,190,250,215,239,63,191,125,111,127,254,207,199,186,243,208,175,125,176,126,57,151,182,124,31,94,11,49,74,212,18,138,145,153,
41,228,88,178,77,49,24,94,241,237,26,94,135,70,246,158,134,219,53,198,184,126,212,202,109,187,14,106,9,192,141,1,227,86,75,201,148,237,50,50,119,1,168,80,98,148,145,106,156,6,149,138,91,186,175,240,234,
181,214,90,225,90,110,230,86,21,80,241,173,106,41,67,44,127,252,175,50,40,52,211,160,252,122,12,160,22,98,192,40,140,28,115,137,91,205,252,26,6,85,133,1,104,140,202,128,242,233,190,46,124,161,144,72,73,
186,136,170,243,247,245,227,219,191,174,95,218,101,79,23,222,151,159,160,47,109,41,231,18,90,252,99,81,225,118,101,148,49,51,83,240,237,26,0,26,250,245,115,144,182,109,45,101,193,243,198,18,248,246,128,
141,243,175,239,37,182,10,181,14,96,144,26,37,160,243,24,35,87,177,140,250,28,160,10,131,91,205,216,68,73,135,215,241,212,141,234,213,175,183,202,109,217,174,139,198,208,208,173,214,234,139,47,148,224,
143,255,24,168,45,149,50,24,15,13,175,26,99,36,146,141,137,197,101,84,63,143,50,252,28,104,148,118,206,129,101,239,49,0,207,244,28,55,20,99,146,82,221,170,179,109,253,163,55,254,230,207,127,241,53,223,
120,12,252,218,62,46,223,239,63,127,61,202,184,46,140,113,179,18,98,140,145,116,94,235,0,13,6,183,175,26,19,5,124,25,80,171,2,52,120,224,91,255,248,248,171,125,78,56,42,229,252,177,165,235,228,182,160,
1,94,63,177,85,240,115,91,146,65,85,240,133,90,143,27,219,200,3,193,42,181,156,111,155,57,139,127,92,199,80,204,111,97,192,31,255,212,109,169,94,41,201,26,114,48,133,161,252,166,196,96,168,44,67,140,58,
84,66,113,109,215,145,131,27,94,203,203,172,169,90,188,14,191,2,165,148,63,31,219,102,27,86,254,252,178,92,63,248,123,138,153,245,177,62,214,191,151,205,30,191,120,60,40,229,254,189,148,178,40,40,148,
50,40,49,74,41,3,178,251,170,56,254,121,29,26,120,29,235,195,63,42,94,55,254,252,235,28,219,11,107,252,76,140,235,199,118,30,94,181,168,4,104,73,214,118,148,58,42,108,243,157,154,191,230,227,103,173,219,
121,169,14,117,235,129,218,167,151,241,55,15,32,85,222,64,120,32,189,100,182,32,111,22,153,162,5,225,60,21,63,161,60,78,3,220,141,134,29,15,35,118,12,239,120,59,133,211,193,86,119,86,118,246,117,239,235,
5,157,118,151,26,123,28,34,49,2,1,179,119,69,202,22,112,15,4,225,129,12,222,49,115,46,142,237,45,53,212,44,222,9,229,201,165,199,190,104,129,158,74,26,51,155,234,211,23,207,119,129,169,140,58,52,220,12,
25,252,219,241,196,246,199,255,30,213,175,101,73,83,236,86,183,154,17,73,74,12,141,81,66,203,24,26,165,12,88,191,0,156,19,146,26,195,63,236,86,107,158,250,92,183,202,86,221,24,130,40,101,248,183,239,203,
118,94,174,127,254,244,203,191,150,205,252,107,253,182,236,176,46,187,226,199,95,133,246,237,47,88,74,25,163,220,174,247,47,37,16,33,74,76,36,215,24,73,114,181,174,63,62,22,180,164,80,172,94,221,120,212,
234,27,21,223,206,3,13,116,179,81,198,64,48,160,12,6,48,134,226,147,167,126,251,48,79,170,2,240,173,146,56,12,36,91,4,239,163,86,167,82,111,215,96,178,227,195,45,207,91,35,61,26,59,128,185,29,234,78,136,
80,254,77,172,59,114,172,115,253,105,144,177,216,35,157,201,61,243,0,34,195,78,253,29,209,8,86,246,238,172,172,190,191,175,14,251,218,212,239,180,254,30,4,1,202,192,161,142,110,230,223,120,246,173,221,
58,106,132,131,59,151,84,125,220,232,194,210,240,83,116,79,119,92,204,137,9,9,66,237,181,71,7,195,252,56,119,205,65,68,188,30,202,96,30,225,139,187,122,60,185,157,101,117,244,150,30,46,205,6,236,9,12,
114,161,95,15,57,136,51,34,223,197,45,51,125,157,195,172,105,224,52,163,201,195,8,204,225,132,55,250,187,237,184,157,88,247,245,29,238,59,157,14,43,30,66,104,143,136,132,15,49,83,49,68,16,18,19,114,209,
157,116,154,188,123,128,100,116,208,10,189,29,58,230,177,107,64,113,144,176,218,235,204,23,239,114,205,59,51,242,38,142,156,226,74,234,201,64,2,166,27,150,99,121,42,175,43,72,241,198,113,10,175,146,188,
165,136,8,66,17,7,232,27,66,204,53,207,127,90,54,236,102,96,199,62,171,137,104,128,97,208,64,23,127,111,96,112,17,6,205,118,223,177,251,59,180,64,113,162,207,53,16,114,37,46,205,77,14,1,58,126,3,115,91,
185,57,102,105,88,9,130,83,207,243,54,59,61,146,231,132,60,68,40,66,228,166,212,233,73,233,17,61,194,113,86,12,92,172,33,210,156,159,46,147,196,129,200,84,156,118,76,196,97,55,34,248,127,1,197,247,15,
190,20,250,30,152,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

const char* editor::scratches_png = (const char*) resource_editor_scratches_png;
const int editor::scratches_pngSize = 23278;
