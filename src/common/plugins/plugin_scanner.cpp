#include <QTimer>

#include "common/plugins/plugin_scanner.h"

class PluginScannerThread : public juce::Thread
{
public:
  PluginScannerThread (ZrythmApp &app, GreeterWidget &greeter)
      : juce::Thread ("GreeterInit"), app_ (app), greeter_ (greeter)
  {
    // autostart
    startThread ();
  }

  // autostop
  ~PluginScannerThread () override { stopThread (-1); }

  void run () override { }
};

PluginScanner::PluginScanner (QObject * parent) : QObject (parent) { }

void
PluginScanner::startScan ()
{
  QTimer::singleShot (0, this, &PluginScanner::performScan);
}

void
PluginScanner::performScan ()
{
  // Simulate work with progress updates
  for (int i = 0; i <= 100; ++i)
    {
      Q_EMIT progressTextChanged (QString ("Processing step %1...").arg (i));
      Q_EMIT progressValueChanged (i / 100.0);
      QThread::msleep (50); // Simulate work
    }
}
