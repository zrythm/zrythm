// ---
//
// $Id: compileroutput.cpp,v 1.3 2005/06/08 08:08:06 nilu Exp $
//
// CppTest - A C++ Unit Testing Framework
// Copyright (c) 2003 Niklas Lundell
//
// ---
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// ---

#include "cpptest-compileroutput.h"
#include "cpptest-source.h"

namespace Test
{
	namespace
	{
		// Checks for output format modifiers.
		//
		bool
		check_format(const std::string& 	    format,
					 std::string::size_type&    pos,
					 const std::string& 		mod,
					 int&				        mod_cnt)
		{
			if (format.compare(pos, mod.size(), mod) == 0)
			{
				if (++mod_cnt > 1)
					throw Test::CompilerOutput::InvalidFormat(format);
				pos += mod.size();
				return true;
			}		
			return false;
		}
		
	} // anonymous namespace
	
	/// Constructs a compiler output handler.
	///
	/// \param format Pre-defined compiler output format.
	/// \param stream Stream to output to.
	///
	CompilerOutput::CompilerOutput(Format format, std::ostream& stream)
	:	Output(),
		_stream(stream)
	{
		static const char* table[] =
		{
			"%file:%line: %text", 					// Generic
			"Error cpptest %file %line: %text",		// BCC
			"%file:%line: %text", 					// GCC
			"%file(%line) : %text"					// MSVC
		};
		
		_format = table[format];
	}
	
	/// Constructs a compiler output handler.
	///
	/// \param format %Output format to use.
	/// \param stream Stream to output to.
	///
	/// \exception InvalidFormat Invalid format specified.
	///
	CompilerOutput::CompilerOutput(const std::string& format, std::ostream& stream)
	:	Output(),
		_format(format),
		_stream(stream)
	{
		int expr(0), file(0), line(0);
		
		for (std::string::size_type pos = 0;
			 (pos = _format.find_first_of('%', pos)) != std::string::npos; )
		{
			++pos;
			if (check_format(_format, pos, "expr", expr))      ;
			else if (check_format(_format, pos, "file", file)) ;
			else if (check_format(_format, pos, "line", line)) ;
			else
				throw InvalidFormat(format);
		}
		
		if (!expr && !file && !line)
			throw InvalidFormat(format);
	}
	
	void
	CompilerOutput::assertment(const Source& s)
	{
		std::string 			fmt(_format);
		std::string::size_type 	pos;
		
		fmt.reserve(fmt.size() + 128);
		
		if ((pos = fmt.find("%file")) != std::string::npos)
			fmt.replace(pos, 5, s.file());
		
		if ((pos = fmt.find("%text")) != std::string::npos)
			fmt.replace(pos, 5, s.message());
		
		if ((pos = fmt.find("%line")) != std::string::npos)
		{
			std::ostringstream ss;
			ss << s.line();
			fmt.replace(pos, 5, ss.str());
		}
		
		_stream << fmt << std::endl;
	}
	
} // namespace Test

