#include "RotatingToggleButton.h"

#if JUCE_MINGW
 #define M_PI 3.14159265358979323846
#endif

RotatingToggleButton::RotatingToggleButton(const String& buttonText):
	ToggleButton(buttonText), rotationState(0.0)
{
	setTriggeredOnMouseDown(true);

    path.startNewSubPath (0.0f, 0.0f);
    path.lineTo (4.0f, 4.0f);
    path.lineTo (0.0f, 8.0f);
    path.closeSubPath();

    setRotationState(0.0);
}



RotatingToggleButton::~RotatingToggleButton()
{
}

void RotatingToggleButton::clicked()
{
	last_timer_call= Time::getMillisecondCounter();
	startTimer(10);
}

void RotatingToggleButton::setRotationState(float rotState)
{
	rotationState= rotState;
	repaint();
}

void RotatingToggleButton::timerCallback()
{
	unsigned time= Time::getMillisecondCounter(), tdiff= time-last_timer_call;
	last_timer_call= time;
	if(getToggleState())
	{
		rotationState+= tdiff*.01;
		if(rotationState>1.0)
		{
			rotationState= 1.0;
			stopTimer();
		}
	}
	else
	{
		rotationState-= tdiff*.01;
		if(rotationState<0.0)
		{
			rotationState= 0.0;
			stopTimer();
		}
	}

	setRotationState(rotationState);
}

void RotatingToggleButton::parentHierarchyChanged()
{
	setRotationState(rotationState);
	ToggleButton::parentHierarchyChanged();
}

void RotatingToggleButton::paintButton (Graphics& g,
										  bool isMouseOverButton,
										  bool isButtonDown)
{
    float fontSize = jmin (15.0f, getHeight() * 0.75f);
    const float tickWidth = fontSize * 1.1f;

    g.setColour(Colour(255, 255, 255));
    AffineTransform transform;
	transform= AffineTransform().translated(-1.0, -(getHeight()-8.0)*.5).
							rotated(rotationState*M_PI*0.5).translated(6.5, (getHeight()-8.0));
    if(isMouseOverButton)
		g.fillPath (path, transform);
    g.strokePath (path, PathStrokeType (1.0f), transform);

    g.setColour (findColour (ToggleButton::textColourId));
    g.setFont (fontSize);

    if (! isEnabled())
        g.setOpacity (0.5f);

    const int textX = (int) tickWidth;

    g.drawFittedText (getButtonText(),
                      textX, 0,
                      getWidth() - textX - 2, getHeight(),
                      Justification::centredLeft, 10);
}

