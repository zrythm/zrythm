#include "PresetComboBox.h"

PresetComboBox::PresetComboBox(const String &name):
	ComboBox(name)
{
	setColour(textColourId, LookAndFeel::getDefaultLookAndFeel().findColour(textColourId));
}

PresetComboBox::~PresetComboBox()
{
	//dtor
}


void PresetComboBox::initItems()
{
	clear();
	int i= 1;
//	addSectionHeading(T("Predefined"));
	addItem(T("Bass Drone 1"), i++);
	addItem(T("Bass Drone 2"), i++);
	addItem(T("Lead Swirl"), i++);
	addItem(T("Preset X"), i++);
	addItem(T("Preset Y"), i++);
//	addSectionHeading(T("User"));
	addSeparator();
	addItem(T("Preset A"), i++);
	addItem(T("Preset B"), i++);
	addItem(T("Preset C"), i++);
	addSeparator();
	addItem(T("Save"), i++);
	addItem(T("Save As..."), i++);
	addItem(T("Delete..."), i++);
}
