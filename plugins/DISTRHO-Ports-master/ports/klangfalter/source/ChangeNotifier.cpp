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

#include "ChangeNotifier.h"

#include <algorithm>


ChangeNotifier::ChangeNotifier() :
  juce::Timer(),
  _listenersMutex(),
  _listeners(),
  _changePending(0),
  _timerInterval(100)
{
  startTimer(_timerInterval);
}
  

ChangeNotifier::~ChangeNotifier()
{
  stopTimer();
  
  juce::ScopedLock lock(_listenersMutex);
  _listeners.clear();
}
 

void ChangeNotifier::notifyAboutChange()
{
  _changePending.set(1);
}

 
void ChangeNotifier::addNotificationListener(ChangeNotifier::Listener* listener)
{
  if (listener)
  {
    juce::ScopedLock lock(_listenersMutex);
    _listeners.insert(listener);
  }
}


void ChangeNotifier::removeNotificationListener(ChangeNotifier::Listener* listener)
{
  if (listener)
  {
    juce::ScopedLock lock(_listenersMutex);
    _listeners.erase(listener);
  }
}
  

void ChangeNotifier::timerCallback()
{  
  if (_changePending.compareAndSetBool(0, 1))
  {
    juce::ScopedLock lock(_listenersMutex);    
    // Some "juggling" with a copy to make sure that the callback can add/remove listeners...
    std::set<Listener*> listeners(_listeners); 
    for (std::set<Listener*>::iterator it=listeners.begin(); it!=listeners.end(); ++it)
    {
      Listener* current = (*it);
      if (_listeners.find(current) != _listeners.end())
      {
        current->changeNotification();
      }
    }
    if (_timerInterval != 40)
    {
      _timerInterval = 40;
      startTimer(_timerInterval);
    }
  }
  else if (_timerInterval != 100)
  {
    _timerInterval = 100;
    startTimer(_timerInterval);
  }
}
