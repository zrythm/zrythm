/*
  ==============================================================================

  This file is part of the dRowAudio JUCE module
  Copyright 2004-13 by dRowAudio.

  ------------------------------------------------------------------------------

  dRowAudio is provided under the terms of The MIT License (MIT):

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.

  ==============================================================================
*/



//==============================================================================
EnvelopeFollower::EnvelopeFollower()
    : envelope (0.0f),
      envAttack (1.0f), envRelease (1.0f)
{
}

EnvelopeFollower::~EnvelopeFollower()
{
}

//==============================================================================
void EnvelopeFollower::processEnvelope (const float* inputBuffer, float* outputBuffer, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        float envIn = fabsf (inputBuffer[i]);
        
        if (envelope < envIn)
            envelope += envAttack * (envIn - envelope);
        else if (envelope > envIn)
            envelope -= envRelease * (envelope - envIn);
        
        outputBuffer[i] = envelope;
    }
}

void EnvelopeFollower::setCoefficients (float attack, float release) noexcept
{
    envAttack = attack;
    envRelease = release;
}

