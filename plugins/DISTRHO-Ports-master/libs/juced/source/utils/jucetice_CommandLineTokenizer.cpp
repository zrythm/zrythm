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
void CommandLineTokenizer::parseCommandLine (const String& commandLine)
{
	int section;
	String value, parse = commandLine.trim();

	while (parse.length() > 0)
	{
		section = parse.indexOf (0, " ");
		if (section == -1)
			section = parse.length();

		argv.add (parse.substring (0, section));

		if (section < parse.length() )
			parse = parse.substring (section+1,parse.length()).trim();
		else
			parse = String();
	}
}

//==============================================================================
const String& CommandLineTokenizer::operator[] (const int index) const throw()
{
	jassert (index >= 0 && index < argv.size());
	return argv[index];
}

const int CommandLineTokenizer::size()
{
	return argv.size();
}

//==============================================================================
int CommandLineTokenizer::searchToken (const String& tokenToSearch, const bool caseSensitive)
{
	if (caseSensitive)
	{
		for (int i = 0; i < argv.size(); i++ )
			if (tokenToSearch.compare (argv[i]) == 0)
				return i;
	}
	else
	{
		for (int i = 0; i < argv.size(); i++ )
			if (tokenToSearch.compareIgnoreCase (argv[i]) == 0)
				return i;
	}
	return -1;
}

//==============================================================================
String CommandLineTokenizer::getOptionString (const String& tokenToSearch,
											  const String& defValue,
											  const bool caseSensitive)
{
	int i = searchToken (tokenToSearch, caseSensitive);
	if (i++ < 0)
		return defValue;

	if (i >= 0 && i < argv.size())
		return argv[i];
	else
		return defValue;
}

bool CommandLineTokenizer::getOptionBool (const String& tokenToSearch,
										  const bool defValue,
										  const bool caseSensitive)
{
	int i = searchToken (tokenToSearch, caseSensitive);
	if (i++ < 0)
		return defValue;

	if (i >= 0 && i < argv.size())
		return argv[i].getIntValue() == 0 ? false : true;
	else
		return defValue;
}

int CommandLineTokenizer::getOptionInt (const String& tokenToSearch,
										const int defValue,
										const bool caseSensitive)
{
	int i = searchToken (tokenToSearch, caseSensitive);
	if (i++ < 0)
		return defValue;

	if (i >= 0 && i < argv.size())
		return argv[i].getIntValue();
	else
		return defValue;
}

double CommandLineTokenizer::getOptionDouble (const String& tokenToSearch,
											  const double defValue,
											  const bool caseSensitive)
{
	int i = searchToken (tokenToSearch, caseSensitive);
	if (i++ < 0)
		return defValue;

	if (i >= 0 && i < argv.size())
		return argv[i].getDoubleValue();
	else
		return defValue;
}

END_JUCE_NAMESPACE
