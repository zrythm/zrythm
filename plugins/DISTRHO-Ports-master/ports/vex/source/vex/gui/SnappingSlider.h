/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2008 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  rockhardbuns
   @tweaker Lucio Asnaghi
   @tweaker falkTX

 ==============================================================================
*/

#ifndef DISTRHO_VEX_SNAPPINGSLIDERCOMPONENT_HEADER_INCLUDED
#define DISTRHO_VEX_SNAPPINGSLIDERCOMPONENT_HEADER_INCLUDED

#ifdef CARLA_EXPORT
 #include "juce_gui_basics.h"
#else
 #include "../../StandardHeader.h"
#endif

class SnappingSlider : public Slider
{
public:
    SnappingSlider(const String& name)
        : Slider(name),
          snapTarget(0.0f),
          snapMargin(0.05f)
    {
    }

    void setSnap(const float target, const float margin)
    {
        snapTarget = target;
        snapMargin = margin;
    }

#if 0
    // FIXME?
    double snapValue(const double attemptedValue , const bool userIsDragging)
    {
        if (std::fabs(attemptedValue - snapTarget) < snapMargin)
            return snapTarget;
        return attemptedValue;
    }
#endif

private:
    float snapTarget, snapMargin;
};

#endif // DISTRHO_VEX_SNAPPINGSLIDERCOMPONENT_HEADER_INCLUDED
