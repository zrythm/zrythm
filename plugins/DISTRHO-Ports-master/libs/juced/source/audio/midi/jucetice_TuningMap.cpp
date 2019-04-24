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
 */

BEGIN_JUCE_NAMESPACE

//==============================================================================
enum TuningMapSection
{
    SEC_None = 0,
    SEC_Unknown = 1,
    SEC_Tuning = 2,
    SEC_ExactTuning = 3
};

extern const String g_equal_tuning;


//==============================================================================
TuningMap::TuningMap()
{
    reset(); // Provide a standard tuning
	
    MemoryInputStream mis((const char*)g_equal_tuning.toUTF8(), g_equal_tuning.length(), false);
	
    readFromStream(mis);
}

TuningMap::~TuningMap()
{
}

//==============================================================================
void TuningMap::reset()
{
    m_dblBaseFreq = 8.1757989156437073336; // Means A = 440Hz
    for ( int i = 0 ; i < 128 ; ++i )
        m_dblTunes[i] = 100 * i;
}


//==============================================================================
bool TuningMap::writeToStream (OutputStream& ofs, bool bSaveBaseFreq)
{
    int i;
    char endl = '\n';
	
    ofs << ";" << endl;
    ofs << "; AnaMark / VAZ 1.5 Plus-compatible tuning map file" << endl;
    ofs << ";" << endl;
    ofs << ";" << endl;
    ofs << "; 1. VAZ-section with quantized tunings" << endl;
    ofs << ";" << endl;
	
    ofs << "[Tuning]" << endl;
    for ( i = 0 ; i < 128 ; ++i )
        ofs << "note " << i << "=" << (int)(m_dblTunes[i]) << endl;
	
    ofs << ";" << endl;
    ofs << "; 2. AnaMark-specific section with exact tunings" << endl;
    ofs << ";" << endl;
    ofs << "[Exact Tuning]" << endl;
	//    ofs.precision(10);
    if ( bSaveBaseFreq )
        ofs << "basefreq = " << m_dblBaseFreq << endl;
    for ( i = 0 ; i < 128 ; ++i )
        ofs << "note " << i << "=" << m_dblTunes[i] << endl;
	
    // Always returns true, because currently there's no error processing
    return true;
}


//==============================================================================
bool TuningMap::readFromStream (InputStream& ifs)
{
    char        szLine[512];
    long        lTunes[128];        // For temporary use, unit: Cents
    bool        bTuningFound = false;
    bool        bExactTuningFound = false;
    long        lET_LastNoteFound = -1;
    long        lLineCount = 0;
    TuningMapSection    secCurr = SEC_None;
	
    // Initialize data
    // Important, because notes not listed in the tuning file
    // should always have standard tuning.
    reset();
    for ( int i = 0 ; i < 128 ; ++i )
        lTunes[i] = (long)m_dblTunes[i];
	
    while ( !ifs.isExhausted() )
    {
        // Increase Line counter to make it easier detecting errors, if
        // a more detailed output is wanted.
        ++lLineCount;
		
        // Read line, until '\n', '\r' or '\0' is reached
        // Thus it is able to read WIN/DOS-style as well as UNIX-style files
        // By the way: Skip empty lines or multiple line-end-characters
        // Is not case sensitive, so all chars are converted to lower ones
        int    nCurrPos = 0;
        do
        {
            while ( (!ifs.isExhausted()) && (nCurrPos < 510) )
            {
                char    ch = '\0';
                ifs.read(&ch, 1);
                if ( (ch == '\0') || (ch == '\r') || (ch == '\n') )
                    break;
                szLine[nCurrPos++] = (char) tolower(ch);
            }
        } while ( (!ifs.isExhausted()) && (nCurrPos == 0) );
        if ( nCurrPos >= 510 )
        {
            // xTRACE( "Line too long - line " << lLineCount );
            return false; // Line too long
        }
        szLine[nCurrPos] = '\0';
		
        // Skip leading spaces/tabs
        const char    * szCurr = szLine;
        while ( (*szCurr != '\0') && ((*szCurr == ' ') || (*szCurr == '\t')) )
            ++szCurr;
		
        // Skip empty lines
        if ( szCurr[0] == '\0' )
            continue;
		
        // Skip comment lines
        if ( szCurr[0] == ';' )
            continue;
		
        // Skip trailing spaces/tabs
        char    * szLast = (char *) &szCurr[strlen(szCurr)-1];
        while ( (szLast > szCurr) && ((*szLast == ' ') || (*szLast == '\t')) )
            --szLast;
        *(szLast+1) = '\0';
		
        // Check for new section
        if ( szCurr[0] == '[' )
        {
            if ( szCurr[strlen(szCurr)-1] != ']' )
            {
                // xTRACE( "Syntax error: Section-tag must be the only string in the line! - line " << lLineCount );
                return false; // error in section-tag! Must be the only one string in the line!
            }
            // Known section found?
            secCurr = SEC_Unknown;
            if ( strcmp(&szCurr[1], "tuning]") == 0 )
            {
                secCurr = SEC_Tuning;
                bTuningFound = true;
            }
            if ( strcmp(&szCurr[1], "exact tuning]") == 0 )
            {
                secCurr = SEC_ExactTuning;
                bExactTuningFound = true;
            }
            // Now process next line
            continue;
        }
		
        // Skip all lines which are in none or in an unknown section
        if ( (secCurr == SEC_None) || (secCurr == SEC_Unknown) )
            continue;
		
        // Separate parameter name and value
        const char    * szParam = szCurr;
        const char    * szValue = strchr(szCurr, '=');
        if ( szValue == 0 )
        {
            // xTRACE( "Syntax error: '=' missing! - line " << lLineCount );
            return false; // definitely an error: '=' missing!
        }
        ++szValue; // Set the pointer to the first char behind the '='
        // Now skip trailing spaces/tabs of parameter name:
        szLast = (char *) &szValue[-2];
        while ( (szLast > szParam) && ((*szLast == ' ') || (*szLast == '\t')) )
            --szLast;
        *(szLast+1) = '\0';
        // Now skip leading spaces/tabs of value:
        while ( (*szValue != '\0') && ((*szValue == ' ') || (*szValue == '\t')) )
            ++szValue;
		
        // Now process the different sections:
        switch ( secCurr )
        {
			case SEC_Tuning:
            {
                // Check for note-tag
                if ( memcmp(szParam, "note", 4) != 0 )
                    continue; // note-tag not found, ignore line in case that it's
				// an option of a later version
				// If you want, you can return here false and process it as an error
				
                // Get MIDI-Note number
                int    lNoteIndex = atol(&szParam[4]);
                // Check for correct range [0;127] and ignore it, if it's out
                // of range.
                if ( (lNoteIndex < 0) || (lNoteIndex > 127) )
                    continue;
                lTunes[lNoteIndex] = atol(szValue);
            }
				break;
				
			case SEC_ExactTuning:
            {
                // Check for note-tag
                if ( memcmp(szParam, "note", 4) == 0 )
                {
                    // note-tag found
                    // Get MIDI-Note number
                    int    lNoteIndex = atol(&szParam[4]);
                    // Check for correct range [0;127] and ignore it, if it's out
                    // of range.
                    if ( (lNoteIndex < 0) || (lNoteIndex > 127) )
                        continue;
                    m_dblTunes[lNoteIndex] = atof(szValue);
                    if ( lET_LastNoteFound < lNoteIndex )
                        lET_LastNoteFound = lNoteIndex;
                    // O.K. -> Process next line
                    continue;
                }
                // Check for basefreq parameter
                if ( strcmp(szParam, "basefreq") == 0 )
                {
                    // basefreq found
                    m_dblBaseFreq = atof(szValue);
                    // O.K. -> Process next line
                    continue;
                }
                // No known parameter found, so skip it
                continue;
            }
				break;
				
			default: // This part of the code should never be reached!
				// assert(false);
				// xTRACE( "Compilation error! Section not coded! - line " << lLineCount );
				return false;
        }
    }
	
    if ( (!bTuningFound) && (!bExactTuningFound) )
    {
        // xTRACE( "No tuning data found!" );
        return false; // No tuning data found at all should be worth an error...
    }
	
    if ( !bExactTuningFound )
    {
        // There are no exact tuning values, so map the quantized
        // values to the exact ones:
        for ( int i = 0 ; i < 128 ; ++i )
            m_dblTunes[i] = (double) lTunes[i];
    }
    else
    {
        // [Exact Tuning] section found, so ignore the values found
        // in the [Tuning] section and do the "auto expand":
        if ( (lET_LastNoteFound >= 0) && (lET_LastNoteFound < 127) )
        {
            // Now loop the given data (auto expand):
            int    H = lET_LastNoteFound;    // Highest MIDI note number
            double    P = m_dblTunes[H];        // Period length
            for ( int i = H ; i < 128 ; ++i )
                m_dblTunes[i] = m_dblTunes[i-H] + P;
        }
    }
	
    return true; // Everything nice!
}

//==============================================================================
bool TuningMap::setBaseFreq (double dblBaseFreq)
{
	//    jassert( dblBaseFreq > 0 );
	
    // First make sure, that the base frequency is in the valid range
    if ( dblBaseFreq > 0 )
    {
        m_dblBaseFreq = dblBaseFreq;
        return true;
    }
    else
    {
        return false;
    }
}

bool TuningMap::setRelativeTune (int nNoteIndex, double dblTune)
{
	//    jassert( (nNoteIndex >= 0) && (nNoteIndex <= 127) );
	
    // First make sure, that the note index is in the valid range
    if ( (nNoteIndex >= 0) && (nNoteIndex <= 127) )
    {
        m_dblTunes[nNoteIndex] = dblTune;
        return true;
    }
    else
    {
        return false;
    }
}

//==============================================================================
const String g_equal_tuning =
"; VAZ Plus 1.5 tuning map file \
; Equal Temperament\
[Tuning]\
note 0=0\
note 1=100\
note 2=200\
note 3=300\
note 4=400\
note 5=500\
note 6=600\
note 7=700\
note 8=800\
note 9=900\
note 10=1000\
note 11=1100\
note 12=1200\
note 13=1300\
note 14=1400\
note 15=1500\
note 16=1600\
note 17=1700\
note 18=1800\
note 19=1900\
note 20=2000\
note 21=2100\
note 22=2200\
note 23=2300\
note 24=2400\
note 25=2500\
note 26=2600\
note 27=2700\
note 28=2800\
note 29=2900\
note 30=3000\
note 31=3100\
note 32=3200\
note 33=3300\
note 34=3400\
note 35=3500\
note 36=3600\
note 37=3700\
note 38=3800\
note 39=3900\
note 40=4000\
note 41=4100\
note 42=4200\
note 43=4300\
note 44=4400\
note 45=4500\
note 46=4600\
note 47=4700\
note 48=4800\
note 49=4900\
note 50=5000\
note 51=5100\
note 52=5200\
note 53=5300\
note 54=5400\
note 55=5500\
note 56=5600\
note 57=5700\
note 58=5800\
note 59=5900\
note 60=6000\
note 61=6100\
note 62=6200\
note 63=6300\
note 64=6400\
note 65=6500\
note 66=6600\
note 67=6700\
note 68=6800\
note 69=6900\
note 70=7000\
note 71=7100\
note 72=7200\
note 73=7300\
note 74=7400\
note 75=7500\
note 76=7600\
note 77=7700\
note 78=7800\
note 79=7900\
note 80=8000\
note 81=8100\
note 82=8200\
note 83=8300\
note 84=8400\
note 85=8500\
note 86=8600\
note 87=8700\
note 88=8800\
note 89=8900\
note 90=9000\
note 91=9100\
note 92=9200\
note 93=9300\
note 94=9400\
note 95=9500\
note 96=9600\
note 97=9700\
note 98=9800\
note 99=9900\
note 100=10000\
note 101=10100\
note 102=10200\
note 103=10300\
note 104=10400\
note 105=10500\
note 106=10600\
note 107=10700\
note 108=10800\
note 109=10900\
note 110=11000\
note 111=11100\
note 112=11200\
note 113=11300\
note 114=11400\
note 115=11500\
note 116=11600\
note 117=11700\
note 118=11800\
note 119=11900\
note 120=12000\
note 121=12100\
note 122=12200\
note 123=12300\
note 124=12400\
note 125=12500\
note 126=12600\
note 127=12700\
";

END_JUCE_NAMESPACE

