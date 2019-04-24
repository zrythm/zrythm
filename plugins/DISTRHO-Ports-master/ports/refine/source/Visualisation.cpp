#include "Visualisation.h"

Visualisation::Visualisation (const RefineDsp& dsp)
    : sepDsp (dsp),
      minMag (-30.f),
      maxMag (0.f)
{
    setOpaque(true);
    startTimer(100);
}

Visualisation::~Visualisation()
{
}

void Visualisation::paint (Graphics& g)
{
    g.fillAll(Colour(0xFF101010));
    const int w = jmin(rmsData.size(), colourData.size(),  getWidth());
    const float yBottom = getHeight() - 1.f;

    for (int i = 0; i<w; ++i)
    {
        const int idx = w - 1 - i;
        const float mag = jlimit(0.f, 1.f, rmsData.getReference(idx));
        const float y = magToY(mag);
        g.setColour(Colour(colourData.getReference(idx)));
        g.drawVerticalLine(i, y, yBottom);
    }
}

void Visualisation::resized()
{
    const int w = getWidth();
    rmsData.clearQuick();
    colourData.clearQuick();
    rmsData.ensureStorageAllocated(w);
    colourData.ensureStorageAllocated(w);

    for (int i = 0; i < w; ++i)
    {
        rmsData.add(0.f);
        colourData.add(0);
    }
}

void Visualisation::timerCallback()
{
    if (sepDsp.getRmsData(rmsData, colourData))
    {

        float newMin = 10;
        float newMax = 0;
        float newMean = 0;
        int meanSize = 0;

        const Colour red(191, 0, 48);
        const Colour green(0, 191, 44);
        const Colour blue(0, 96, 191);

        for (int i = 0; i<rmsData.size(); ++i)
        {
            if (rmsData.getReference(i) < 1e-8f)
            {
                rmsData.getReference(i) = -80.f;
                continue;
            }

            newMean += rmsData.getReference(i);
            ++meanSize;

            const float curMean = newMean / meanSize;
            const float x = rmsData.getReference(i);
            const float weight = 0.2f + 0.8f * sqrt(1.f - i / (rmsData.size() - 1.f));

            const float xMin = curMean - weight * (curMean - x);
            const float xMax = curMean + weight * (x - curMean);

            newMin = jmin(newMin, xMin);
            newMax = jmax(newMax, xMax);

            rmsData.getReference(i) = 10 * log10(rmsData.getReference(i));

            {
                const float mulBlue = ((colourData.getReference(i) >> 16) & 0xFF) / 255.f;
                const float mulGreen = ((colourData.getReference(i) >> 8) & 0xFF) / 255.f;
                const float mulRed = ((colourData.getReference(i)) & 0xFF) / 255.f;

                uint8_t r = uint8_t(blue.getRed()*mulBlue + green.getRed()*mulGreen + red.getRed()*mulRed);
                uint8_t g = uint8_t(blue.getGreen()*mulBlue + green.getGreen()*mulGreen + red.getGreen()*mulRed);
                uint8_t b = uint8_t(blue.getBlue()*mulBlue + green.getBlue()*mulGreen + red.getBlue()*mulRed);

                r = 64 + r / 2;
                g = 64 + g / 2;
                b = 64 + b / 2;

                colourData.getReference(i) = (255 << 24) | (r << 16) | (g << 8) | (b << 0);
            }
        }

        if (newMin < 10 && newMin > 0)
        {
            newMean /= meanSize;

            const float dist = jmax(0.01f, jmin(newMax - newMean, newMean - newMin));

            newMin = jmax(1e-8f, jmin(0.9f*newMin, newMean - dist));
            newMax = jmax(1.1f*newMax, newMean + dist);

            const float minDb = 10 * std::log10(newMin);
            const float maxDb = 10 * std::log10(newMax);

            minMag = minMag*0.95f + 0.05f*minDb;
            maxMag = maxMag*0.95f + 0.05f*maxDb;
        }

        for (int i = 0; i<rmsData.size(); ++i)
            rmsData.getReference(i) = jlimit(0.f, 1.f, float((rmsData.getReference(i) - minMag) / (maxMag - minMag)));
        
        repaint();
    }
}

float Visualisation::magToY (float mag) const
{
    return 1.f + (getHeight() - 2.f) * (1.f - mag);
}
