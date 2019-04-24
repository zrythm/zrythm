#ifndef ROTATINGTOGGLEBUTTON_H
#define ROTATINGTOGGLEBUTTON_H

#include "juce_PluginHeaders.h"

class RotatingToggleButton : public ToggleButton, public Timer
{
	public:
		RotatingToggleButton(const String& buttonText);
		virtual ~RotatingToggleButton() override;
		virtual void setRotationState(float rotationState);
	protected:
		/** @internal */
		void paintButton (Graphics& g,
						  bool isMouseOverButton,
						  bool isButtonDown) override;

		void clicked() override;

		void timerCallback() override;

		void parentHierarchyChanged() override;

		unsigned last_timer_call;

		Path path;
		float rotationState;
	private:
};

#endif // ROTATINGTOGGLEBUTTON_H
