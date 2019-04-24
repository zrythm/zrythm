// ---
//
// $Id: cpptest-compileroutput.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
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

/** \file */

#ifndef CPPTEST_COMPILEROUTPUT_H
#define CPPTEST_COMPILEROUTPUT_H

#include "cpptest-output.h"

namespace Test
{
	/// \brief Compiler-like output handler.
	///
	/// %Test suite output handler that only outputs failures in compiler
	/// warning/error format. This way, you can use your IDE to browse between
	/// failures.
	///
	/// The output format is configurable to be able to emulate different
	/// compiler outputs. The following modifiers exist:
	/// - \e %file Outputs the file containing the test function.
	/// - \e %line Line number for the the test function.
	/// - \e %text Expression (or message) that caused the assertment.
	/// Note that each modifier can only be specified once.
	///
	class CompilerOutput : public Output
	{
	public:
		/// \brief Compiler output exception.
		///
		/// Indicates that an invalid message format was given when creating
		/// a compiler output. The failing format may be retrieved using the
		/// what() method.
		///
		class InvalidFormat : public std::logic_error
		{
			public:
				InvalidFormat(const std::string& what)
					: std::logic_error(what) {}
		};
		
		/// Pre-defined compiler output formats.
		///
		enum Format
		{
			/// Generic compiler format, which equals:
			/// <tt>%%file:%%line: %%text</tt>
			/// 
			Generic,

			/// <a href="http://www.borland.com/products/downloads/download_cbuilder.html">
			/// Borland C++ Compiler</a> (BCC) format, which equals:
			/// <tt>Error cpptest %%file %%line: %%text</tt>.
			///
			BCC,
			
			/// <a href="http://gcc.gnu.org">GNU Compiler Collection</a> 
			/// (GCC) format, which equals:
			/// <tt>%%file:%%line: %%text</tt>
			///
			GCC,

			/// <a href="http://www.microsoft.com">Microsoft Visual C++</a> 
			/// (MSVC) format, which equals:
			/// <tt>%%file(%%line) : %%text</tt>
			///
			MSVC
		};
		
		explicit CompilerOutput(Format 				format = Generic,
								std::ostream& 		stream = std::cout);
		
		explicit CompilerOutput(const std::string&	format,
								std::ostream& 		stream = std::cout);
		
		virtual void assertment(const Source& s);
		
	private:
		std::string		_format;
		std::ostream&	_stream;
	};

} // namespace Test
	
#endif // #ifndef CPPTEST_COMPILEROUTPUT_H

