#ifndef __PRESETMANAGER__
#define __PRESETMANAGER__

#include "JuceHeader.h"

class PresetManager
{
public:

	PresetManager(AudioProcessor* processor_, File presetfile)
		: processor(processor_),
		  presetFile(presetfile)
	{
		presetFile.getParentDirectory().createDirectory();
		presetFile.create();
		scanPresets();
	}

	void loadPreset(String presetName)
	{
		if (xmlData != nullptr)
		{
			forEachXmlChildElementWithTagName(*xmlData, e, "PRESET")
			{
				if (e->getStringAttribute("name") == presetName)
				{
					XmlElement* xml = e->getChildElement(0);

					if (xml != nullptr)
					{
						String xmlString(xml->createDocument("", true, false));
						const int stringLength = xmlString.getNumBytesAsUTF8();
						MemoryBlock mb((size_t) stringLength + 9);

						char* const d = static_cast<char*> (mb.getData());
						*(uint32*) d = ByteOrder::swapIfBigEndian ((const uint32) 0x21324356);
						*(uint32*) (d + 4) = ByteOrder::swapIfBigEndian ((const uint32) stringLength);

						xmlString.copyToUTF8(d + 8, stringLength + 1);

						processor->setStateInformation(mb.getData(), (int) mb.getSize());
					}
					break;
				}
			}
		}
	}

	void removePreset(const String presetName, bool rescan)
	{
		if (xmlData != nullptr)
		{
			forEachXmlChildElementWithTagName(*xmlData, e, "PRESET")
			{
				if (e->getStringAttribute("name") == presetName)
				{
					xmlData->removeChildElement(e, true);
					break;
				}
			}
		}

		xmlData->writeToFile(presetFile, "");
		
		if (rescan)
			scanPresets();
	}


	void storePreset(const String& presetName)
	{
		if (presetNames.contains(presetName))
			removePreset(presetName, false);

		{
			MemoryBlock mb;
			processor->getStateInformation(mb);

			char* cdata = (char*) mb.getData();			

			String data(String::fromUTF8(cdata+8, (int) mb.getSize() - 8));

			XmlDocument xmlDoc(data);
			XmlElement* xml = xmlDoc.getDocumentElement();

			if (xml != nullptr)
			{
				XmlElement* e = xmlData->createNewChildElement("PRESET");
				e->setAttribute("name", presetName);
				e->addChildElement(xml);
			}
		}

		xmlData->writeToFile(presetFile, "");
		scanPresets();
	}

	StringArray getPresetNames()
	{
		return presetNames;
	}

private:

	void scanPresets()
	{
		XmlDocument xmlDoc(presetFile);
		xmlData = xmlDoc.getDocumentElement();
		presetNames.clear();

		if (xmlData != nullptr && ! xmlData->hasTagName("PRESETS") )
			xmlData = nullptr;

		if (xmlData == nullptr)
			return;

		presetNames.clear();

		forEachXmlChildElementWithTagName(*xmlData, e, "PRESET")
		{
			presetNames.add(e->getStringAttribute("name"));
		}
	}

	AudioProcessor* processor;
	File presetFile;

	ScopedPointer<XmlElement> xmlData;
	StringArray presetNames;
};



#endif