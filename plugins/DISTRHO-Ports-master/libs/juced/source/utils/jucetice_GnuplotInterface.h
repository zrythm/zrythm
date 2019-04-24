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

#ifndef __JUCETICE_GNUPLOTINTERFACE_HEADER__
#define __JUCETICE_GNUPLOTINTERFACE_HEADER__

#if JUCE_LINUX

//==============================================================================
/**
    Gnuplot interface class.
    
    This let you plot data directly from the application (obviously you have
    to install gnuplot package before using this class).
    
    This is fully functional in linux only.
    
    @code
        Gnuplot myPlot;
        myPlot.setMultiplot (true);

        Array<double> data1 (1000);
        for (int j = 0; j != 1000; ++j)
        {
            const double jD = static_cast<double>(j);
            data1.set (j, sin (jD / 100.0));
        }

        myPlot.plot (data1);
    @endcode
*/
class Gnuplot
{
public:

    //==============================================================================
    /** Styles of the plot */
    enum GnuplotStyle
    {
      styleLines,
      stylePoints,
      styleLinespoints,
      styleImpulses,
      styleDots,
      styleSteps,
      styleErrorbars,
      styleBoxes,
      styleBoxerrorbars,
    };

    /** Output type for the plot */
    enum GnuplotTerminal
    {
      terminalX11,
      terminalWxt,
      terminalPng
    };

    //==============================================================================
    /** Costructor */
    Gnuplot();
    
    /** Destructor */
    ~Gnuplot();

    //==============================================================================
    void addWindow (const int nWindows = 1);
    void changeWindow (const int);

    //==============================================================================
    void setTerminal (const GnuplotTerminal&);
    void setOutput (const String& output);
    void setStyle (const GnuplotStyle&);
    void setMultiplot (const bool isMultiple);
    void setYlabel (const String&);
    void setXlabel (const String&);

    //==============================================================================
    void plot (const String& file, const String& title = "y");
    void plot (const Array<double>& x, const String& title = "y");
    void plot (const Array<double>& x, const Array<double>& y, const String& title = "data");
    void plot (const Array<double>& x, const Array<double>& y, const Array<double>& z, const String& title = "z");

    void emptyPlot();

private:

    void* mGnuPipe;
    
    String mTerminal;
    String mStyle;
    Array<StringArray> mWindowData;

    int mCurrentWindowNumber;
    static int mTempFileNumber;

    void setLineStyles();
    const String readFileName() const;
    const String createTempFileName();
    void createTempFile (const String&, const Array<double>&);
    void createTempFile (const String&, const Array<double>&, const Array<double>&);
    void createTempFile (const String&, const Array<double>&, const Array<double>&, const Array<double>&);
    void plotTempFile2D (const String&, const String&);
    void plotTempFile3D (const String&, const String&);
    void execute (const String&) const;
};

#endif 

#endif
