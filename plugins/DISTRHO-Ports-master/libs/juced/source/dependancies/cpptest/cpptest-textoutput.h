// ---
//
// $Id: cpptest-textoutput.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
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

#ifndef CPPTEST_TEXTOUTPUT_H
#define CPPTEST_TEXTOUTPUT_H

#include "cpptest-source.h"
#include "cpptest-output.h"

namespace Test
{
	/// \brief Text output handler that outputs to the a stream.
	///
	/// %Test suite output handler that writes its information as text to a
	/// a stream. It it possible to select between two different operational
	/// modes that controls the detail level, see Mode.
	///
	class TextOutput : public Output
	{
	public:
		/// Output mode.
		///
		enum Mode
		{
			/// Terse output mode, which only shows the number of correct tests.
			///
			Terse,
			
			/// Verbose output mode, which also shows extended assert
			/// information for each test that failed.
			///
			Verbose			
		};
		
		TextOutput(Mode mode, std::ostream& stream = std::cout);
		
		virtual void finished(int tests, const Time& time);
		virtual void suite_start(int tests, const std::string& name);
		virtual void suite_end(int tests, const std::string& name,
							   const Time& time);
		virtual void test_end(const std::string& name, bool ok,
							  const Time& time);
		virtual void assertment(const Source& s);
		
	private:
		typedef std::list<Source> ErrorList;
		
		Mode 			_mode;
        std::ostream& 	_stream;
		ErrorList		_suite_error_list;
		std::string		_suite_name;
		int				_suite_errors;
		int				_suite_tests;
		int				_suite_total_tests;
		int				_total_errors;
	};

} // namespace Test
	
#endif // #ifndef CPPTEST_TEXTOUTPUT_H
