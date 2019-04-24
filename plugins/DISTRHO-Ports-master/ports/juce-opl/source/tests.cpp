#include "JuceHeader.h"
#include "EnumFloatParameter.h"

class EnumFloatParameterTest : public UnitTest
{
public:

	EnumFloatParameterTest() : UnitTest("EnumFloatParameterTest") {}

	void runTest()
	{
		beginTest("Test enumerated parameters");
		const String percussion[] = { "Off", "Bass drum", "Snare", "Tom", "Cymbal", "Hi-hat" };
		int n = sizeof(percussion) / sizeof(String);
		EnumFloatParameter* efp = new EnumFloatParameter("Percussion Mode", StringArray(percussion, n));
		expect(true);
		for (int i = 0; i < n; i++) {
			efp->setParameterIndex(i);
			expect(i == efp->getParameterIndex());
		}
	}

};

static EnumFloatParameterTest enumTest;

