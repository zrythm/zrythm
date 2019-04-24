// ---
//
// $Id: cpptest-htmloutput.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
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

#ifndef CPPTEST_HTMLOUTPUT_H
#define CPPTEST_HTMLOUTPUT_H

#include "cpptest-collectoroutput.h"

namespace Test
{
	/// \brief HTML output.
	///
	/// %Output handler that creates a HTML table with detailed information
	/// about all tests.
	///
	class HtmlOutput : public CollectorOutput
	{
	public:
		void generate(std::ostream& os, bool incl_ok_tests = true, 
					  const std::string& name = "");
		
	private:
		struct SuiteRow;
		struct TestRow;
		struct TestSuiteRow;
		struct TestResult;
		struct TestResultAll;
		struct SuiteTestResult;

		friend struct TestSuiteRow;
	};
	
} // namespace Test
		
#endif // #ifndef CPPTEST_HTMLOUTPUT_H		

