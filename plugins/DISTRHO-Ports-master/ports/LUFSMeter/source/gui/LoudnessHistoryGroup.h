/*
 ==============================================================================

 LoudnessHistoryGroup.h
 
 Created: 13 Jan 2014 11:13:44pm
 Author:  Samuel Gaehwiler
 
 This file is part of the LUFS Meter audio measurement plugin.
 Copyright 2011-14 by Klangfreund, Samuel Gaehwiler.
 
 -------------------------------------------------------------------------------
 
 The LUFS Meter can be redistributed and/or modified under the terms of the GNU
 General Public License Version 2, as published by the Free Software Foundation.
 A copy of the license is included with these source files. It can also be found
 at www.gnu.org/licenses.
 
 The LUFS Meter is distributed WITHOUT ANY WARRANTY.
 See the GNU General Public License for more details.
 
 -------------------------------------------------------------------------------
 
 To release a closed-source product which uses the LUFS Meter or parts of it,
 a commercial license is available. Visit www.klangfreund.com/lufsmeter for more
 information.

 ==============================================================================
*/

#ifndef LOUDNESSHISTORYGROUP_H_INCLUDED
#define LOUDNESSHISTORYGROUP_H_INCLUDED

#include "../MacrosAndJuceHeaders.h"

//==============================================================================
/**
 A parent component and timer for LoudnessHistory instances.
 
 All instances of LoudnessHistory should be children of the same instance of this
 class.
 
 (If every LoudnessHistory would be its own timer, they would not be refreshed at
 the same time, which looks ugly.)
 */
class LoudnessHistoryGroup  : public Component,
                              public Timer
{
public:
    LoudnessHistoryGroup ();
    
    ~LoudnessHistoryGroup ();
    
    void timerCallback () override;
    void resized () override;
    void paint (Graphics& g) override;
    
private:
    
};



#endif  // LOUDNESSHISTORYPARENT_H_INCLUDED
