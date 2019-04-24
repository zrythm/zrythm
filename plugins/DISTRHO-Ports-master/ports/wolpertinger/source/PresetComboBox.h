#ifndef PRESETCOMBOBOX_H
#define PRESETCOMBOBOX_H

#include "juce_PluginHeaders.h"


class PresetComboBox : public ComboBox
{
	public:
		PresetComboBox(const String &name);
		virtual ~PresetComboBox() override;
		void initItems();
	protected:
	private:
};

#endif // PRESETCOMBOBOX_H
