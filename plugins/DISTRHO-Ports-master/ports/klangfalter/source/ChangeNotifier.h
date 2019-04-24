// ==================================================================================
// Copyright (c) 2012 HiFi-LoFi
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ==================================================================================

#ifndef _CHANGENOTIFIER_H
#define _CHANGENOTIFIER_H

#include "JuceHeader.h"

#include <set>


/**
* @class ChangeNotifier
* @brief Notifies listeners within a certain time interval about changes
*
* The difference to the juce::ChangeBroadcaster is that notifying about
* changes doesn't involve any blocking calls (e.g. malloc() etc.), so it
* safe to call even from realtime threads.
*/
class ChangeNotifier : public juce::Timer
{
public:
  ChangeNotifier();  
  virtual ~ChangeNotifier();
  
  void notifyAboutChange();
  
  /**
  * @class Listener
  * @brief Interface for class which want to be notified about changes
  */
  class Listener
  {
  public:
    virtual ~Listener()
    {
    }
    
    virtual void changeNotification() = 0;
  };
  
  void addNotificationListener(Listener* listener);
  
  void removeNotificationListener(Listener* listener);
  
  virtual void timerCallback();
  
private:
  juce::CriticalSection _listenersMutex;
  std::set<Listener*> _listeners;
  juce::Atomic<juce::int32> _changePending;
  int _timerInterval;
  
  ChangeNotifier (const ChangeNotifier&);
  ChangeNotifier& operator=(const ChangeNotifier&);
};

#endif // Header guard
