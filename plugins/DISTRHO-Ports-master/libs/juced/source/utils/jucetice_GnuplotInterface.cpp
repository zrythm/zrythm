/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

    Created by Richel Bilderbeek on Fri Jun 10 2005.

 ==============================================================================
*/

#if JUCE_LINUX

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>

BEGIN_JUCE_NAMESPACE

//==============================================================================

//#define EXTENSIVE_LOG
int Gnuplot::mTempFileNumber = 0;

//==============================================================================
Gnuplot::Gnuplot() :
	mGnuPipe (0),
	mTerminal ("wxt"),
	mStyle ("lines"),
	mCurrentWindowNumber(0)
{
  File file (readFileName());
  if (! file.exists ())
  {
    printf ("Gnuplot executable not found.\n");
    return;
  }

  printf ("Gnuplot executable found at '%s'.\n", (const char*) file.getFullPathName().toUTF8());
  printf ("Opening pipe to Gnuplot\n");

  String fileName = file.getFullPathName ();
  mGnuPipe = ::popen ((const char*) (fileName + " -persist").toUTF8(), "w"); //Changed 'rw' to 'w'
  if (mGnuPipe==0)
  {
    printf ("Couldn't open connection to gnuplot\n");
    return;
  }

  setLineStyles ();
  addWindow ();
  changeWindow (0);
}

Gnuplot::~Gnuplot()
{
  for (int i = 0; i != mWindowData.size(); ++i)
  {
    mCurrentWindowNumber = i;
    emptyPlot();
  }
  if (::pclose ((FILE*) mGnuPipe) == -1)
  {
    jassert(!"Problem closing communication to Gnuplot");
  }
}

//==============================================================================
const String Gnuplot::readFileName() const
{
  String returnFileName;
  returnFileName ="/usr/bin/gnuplot";
  return returnFileName;
}

//==============================================================================
void Gnuplot::emptyPlot()
{
  const int nPlots = mWindowData[mCurrentWindowNumber].size();
  for (int i = 0; i != nPlots; ++i)
  {
    DBG ("Removing file: " + mWindowData[mCurrentWindowNumber][i]);
    File file (mWindowData[mCurrentWindowNumber][i]);
    file.deleteFile ();
  }
}

//==============================================================================
void Gnuplot::setTerminal (const GnuplotTerminal& terminal)
{
  switch(terminal)
  {
    case terminalPng  : mTerminal = "png"; break;
    case terminalWxt  : mTerminal = "wxt"; break;
    case terminalX11  : mTerminal = "x11" ; break;
    default: jassert (!"Unknown GnuplotTerminal");
  }
}

void Gnuplot::setStyle (const GnuplotStyle& style)
{
  switch(style)
  {
    case styleLines        : mStyle = "lines"; break;
    case stylePoints       : mStyle = "points"; break;
    case styleLinespoints  : mStyle = "linespoints"; break;
    case styleImpulses     : mStyle = "impulses"; break;
    case styleDots         : mStyle = "dots"; break;
    case styleSteps        : mStyle = "steps"; break;
    case styleErrorbars    : mStyle = "errorbars"; break;
    case styleBoxes        : mStyle = "boxes"; break;
    case styleBoxerrorbars : mStyle = "boxerrorbars"; break;
    default: jassert (!"Unknown GnuplotStyle");
  }
}

void Gnuplot::setYlabel (const String& label)
{
  const String command = "set ylabel \""+label+"\"";
  execute (command);
}

void Gnuplot::setXlabel (const String& label)
{
  const String command = "set xlabel \""+label+"\"";
  execute (command);
}

void Gnuplot::setOutput (const String& output)
{
  const String command = "set output \""+output+"\"";
  execute (command);
}

void Gnuplot::setMultiplot (const bool isMultiple)
{
  const String command = (isMultiple ? "set multiplot" : "unset multiplot");
  execute (command);
}

//==============================================================================
void Gnuplot::changeWindow(const int windowNumber)
{
  if (windowNumber == mCurrentWindowNumber) return;
  if (windowNumber >= static_cast<int>(mWindowData.size()))
  {
    addWindow();
    mCurrentWindowNumber = mWindowData.size()-1;
  }
  else
  {
    mCurrentWindowNumber = windowNumber;
  }
  const String myCommand = "set terminal " + mTerminal + " " + String (mCurrentWindowNumber + 1);
  execute (myCommand);
}

void Gnuplot::addWindow (const int nWindows)
{
  for (int i = 0; i != nWindows; ++i)
  {
    StringArray temp;
    mWindowData.add (temp);
  }
}

//==============================================================================
void Gnuplot::plot (const String &equation, const String &title)
{
  String myCommand;
  if (mWindowData[mCurrentWindowNumber].size() > 0)
    myCommand = "replot " + equation + " title \"" + title + "\" with " + mStyle;
  else
    myCommand = "plot " + equation + " title \"" + title + "\" with " + mStyle;

  execute (myCommand);
}

void Gnuplot::plot (const Array<double>& x, const String &title)
{
  const String tempFileName = createTempFileName();
  createTempFile (tempFileName, x);
  plotTempFile2D (tempFileName, title);
}

void Gnuplot::plot (const Array<double>& x, const Array<double>& y, const String &title)
{
  const String tempFileName = createTempFileName();
  createTempFile (tempFileName, x, y);
  plotTempFile2D (tempFileName, title);
}

void Gnuplot::plot (const Array<double>& x, const Array<double>& y, const Array<double>& z, const String &title)
{
  const String tempFileName = createTempFileName();
  createTempFile (tempFileName, x, y, z);
  plotTempFile3D (tempFileName, title);
}

void Gnuplot::plotTempFile2D (const String& tempFileName, const String& title)
{
  String myCommand;
  const int nSeries = mWindowData[mCurrentWindowNumber].size();

/*
  //Linestyles are nice, but don't work on my terminal...
  ///if (nSeries>12) printf ("MAKE MORE LINESTYLES!!!\n");
  //if ( nSeries == 0)
  //	myCommand = "plot \"" + tempFileName + "\" ls "+String(nSerie)+" title \"" + title + "\" with " + mStyle;
  //else
  //	myCommand = "replot \"" + tempFileName + "\" ls "+String(nSerie)+" title \"" + title + "\" with " + mStyle;
*/

  if ( nSeries == 0)
    myCommand = "plot \"" + tempFileName + "\" title \"" + title + "\" with " + mStyle;
  else
    myCommand = "replot \"" + tempFileName + "\" title \"" + title + "\" with " + mStyle;

  execute (myCommand);
  Thread::sleep (1000);
  mWindowData[mCurrentWindowNumber].add (tempFileName);
}

void Gnuplot::plotTempFile3D (const String& tempFileName, const String& title)
{
  //Does not work on my computer. Should be something like this...
  execute ("set pm3d");
  String myCommand;
  if (mWindowData[mCurrentWindowNumber].size() == 0)
    myCommand = "splot \"" + tempFileName + "\" title \"" + title + "\" with pm3d palette";
  else
    myCommand = "replot \"" + tempFileName + "\" title \"" + title + "\" with pm3d palette";

  execute (myCommand);

  Thread::sleep (1000);
  mWindowData[mCurrentWindowNumber].add(tempFileName);
}

//==============================================================================
const String Gnuplot::createTempFileName()
{
  ++mTempFileNumber;
  const File tempFile = File::createTempFile ("GnuplotTemp" + String (mTempFileNumber));
  DBG ("Created temp file '" + tempFile.getFullPathName () + "'");
  return tempFile.getFullPathName ();
}

void Gnuplot::createTempFile (const String& tempFileName,
                              const Array<double>& x)
{
  File file (tempFileName);
  FileOutputStream* out = file.createOutputStream ();

  const int size = x.size();
  for (int i = 0; i != size; i++)
    *out << x[i] << "\n";
  
  delete out;
}

void Gnuplot::createTempFile (const String& tempFileName,
                              const Array<double>& x,
                              const Array<double>& y)
{
  jassert (x.size() == y.size());

  File file (tempFileName);
  FileOutputStream* out = file.createOutputStream ();

  const int size = x.size();
  for (int i = 0; i != size; i++)
    *out << x[i] << " " << y[i] << "\n";

  delete out;
}

void Gnuplot::createTempFile (const String& tempFileName,
                              const Array<double>& x,
                              const Array<double>& y,
                              const Array<double>& z)
{
  jassert (x.size() == y.size());
  jassert (y.size() == z.size());

  File file (tempFileName);
  FileOutputStream* out = file.createOutputStream ();

  const int size = x.size();
  for (int i = 0; i != size; i++)
    *out << x[i] << " " << y[i] << " " << z[i] << "\n";

  delete out;
}

//==============================================================================
void Gnuplot::setLineStyles()
{
  //Does not work on all terminals. Like mine... :-(
  //Execute("set style line  1 lt pal frac 0.0 lw 2");
  //Execute("set style line  2 lt pal frac 0.0 lw 4");
  //Execute("set style line  3 lt pal frac 0.2 lw 2");
  //Execute("set style line  4 lt pal frac 0.2 lw 4");
  //Execute("set style line  5 lt pal frac 0.4 lw 2");
  //Execute("set style line  6 lt pal frac 0.4 lw 4");
  //Execute("set style line  7 lt pal frac 0.6 lw 2");
  //Execute("set style line  8 lt pal frac 0.6 lw 4");
  //Execute("set style line  9 lt pal frac 0.8 lw 2");
  //Execute("set style line 10 lt pal frac 0.8 lw 4");
  //Execute("set style line 11 lt pal frac 1.0 lw 2");
  //Execute("set style line 12 lt pal frac 1.0 lw 4");
}

//==============================================================================
void Gnuplot::execute(const String& cmdstr) const
{
  DBG ("Command sent to Gnuplot: " + cmdstr);
  ::fputs ((const char*) (cmdstr+"\n").toUTF8(), (FILE*) mGnuPipe);
  ::fflush ((FILE*) mGnuPipe);

  Thread::sleep (1000);
}

END_JUCE_NAMESPACE

#endif

