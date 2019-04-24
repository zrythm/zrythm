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

#ifndef __JUCETICE_COMMANDLINETOKENIZER_HEADER__
#define __JUCETICE_COMMANDLINETOKENIZER_HEADER__

//==============================================================================
/** Utility class to tokenize a commandLine

*/
class CommandLineTokenizer
{
public:

	CommandLineTokenizer () {}
	~CommandLineTokenizer () {}

    //==============================================================================
    /** Handy utility to create a tokenized string

        If you pass an empty string, nothing will be done, every values are returned
		as empty strings
    */
    void parseCommandLine (const String& commandLine);

	//==============================================================================
    /** Handy functions to get values as like argv[x]

        Since the application name isn't used in juce the index of the first
        value is obviously zero.
	*/
	const String& operator[] (const int index) const throw();

    //==============================================================================
    /** Get the numbers of actual string in the array */
	const int size();

    //==============================================================================
    /** Handy utility to check for an existing value in the line

        If are looking for an option switch it return the index of the switch
		token in the command line otherwise -1.
		Optionally you can make the option check not to be case sensitive.
    */
    int searchToken (const String& tokenToSearch, const bool caseSensitive = true);

    //==============================================================================
	/** Variables getter with casting */
	String getOptionString (const String& tokenToSearch,
							const String& defValue = String(),
							const bool caseSensitive = true);

	bool getOptionBool (const String& tokenToSearch,
						const bool defValue = false,
						const bool caseSensitive = true);

	int getOptionInt (const String& tokenToSearch,
						const int defValue = 0,
						const bool caseSensitive = true);

	double getOptionDouble (const String& tokenToSearch,
							const double defValue = 0.0,
							const bool caseSensitive = true);


private:

	StringArray argv;
};


#endif // __JUCETICE_COMMANDLINETOKENIZER_HEADER__

