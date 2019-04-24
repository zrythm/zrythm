#include "ReFineLookAndFeel.h"

void RefineLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider& slider)
{
    if (RefinedSlider* s = dynamic_cast<RefinedSlider*> (&slider))
    {
        const Image& sliderImg = s->sliderImage;
        const Image& vuImg = s->vuImage;
        jassert(sliderImg.getWidth() == vuImg.getWidth() && sliderImg.getHeight() == vuImg.getHeight());

        const int wSubImage = sliderImg.getWidth();
        const int numImages = sliderImg.getHeight() / wSubImage;
        const int ySliderImage = wSubImage * jlimit(0, numImages - 1, static_cast<int> (sliderPosProportional * (numImages - 1) + 0.5f));
        const int yVuImage = wSubImage * jlimit(0, numImages - 1, static_cast<int> (s->getVuValue() * (numImages - 1) + 0.5f));

        g.drawImage(sliderImg, x, y, width, height, 0, ySliderImage, wSubImage, wSubImage, false);
        g.drawImage(vuImg, x, y, width, height, 0, yVuImage, wSubImage, wSubImage, false);
    }
}