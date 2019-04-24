#ifndef KEYBOARDBUTTON_H
#define KEYBOARDBUTTON_H

#include "RotatingToggleButton.h"


class KeyboardButton : public RotatingToggleButton
{
	public:
		KeyboardButton(const String& buttonText);
		virtual ~KeyboardButton() override;
		void setRotationState(float rotState) override;
	protected:
		void parentHierarchyChanged() override;
		int parentHeight;
	private:
};

#endif // KEYBOARDBUTTON_H
