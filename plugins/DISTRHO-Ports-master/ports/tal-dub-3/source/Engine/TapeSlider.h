#if !defined(__TAPE_SLIDER_h)
#define __TAPE_SLIDER_h

/*-----------------------------------------------------------------------------

Kunz Patrick 30.04.2007

Slides linear from one value to the next.

-----------------------------------------------------------------------------*/

class TapeSlider 
{
private:
	float speed;
	float value;
	float newValue;

public:

	TapeSlider(float sampleRate)
	{
		speed = 0.3f / (44100.0f * sampleRate / 44100.0f); // 0.5s
		newValue = 0.0f;
		value= 0.0f;
	}

	// any inValue
	inline float tick(float inValue) 
	{
		if (value == inValue)
		{
			return value;
		}
		if (value > inValue)
		{ 
			value -= speed;
			if (value < inValue)
			{
				value = inValue;
			}
		}
		else if (value < inValue)
		{
			value += speed;
			if (value > inValue)
			{
				value = inValue;
			}
		}
		return value;
	}
};
#endif
