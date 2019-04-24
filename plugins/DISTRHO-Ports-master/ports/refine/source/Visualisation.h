#ifndef VISUALISATION_H_INCLUDED
#define VISUALISATION_H_INCLUDED

#include "JuceHeader.h"
#include "RefineDsp.h"

class Visualisation    : public Component, public Timer
{
public:
    Visualisation (const RefineDsp& dsp);
    ~Visualisation();

    void paint (Graphics& g);
    void resized();
    void timerCallback();
    float magToY (float mag) const;

private:

    const RefineDsp& sepDsp;
    Array<float> rmsData;
    Array<uint32> colourData;

    float minMag;
    float maxMag;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Visualisation)
};


#endif  // VISUALISATION_H_INCLUDED
