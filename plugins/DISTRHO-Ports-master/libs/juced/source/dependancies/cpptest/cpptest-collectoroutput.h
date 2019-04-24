// ---
//
// $Id: cpptest-collectoroutput.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
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

#ifndef CPPTEST_COLLECTOROUTPUT_H
#define CPPTEST_COLLECTOROUTPUT_H

#include "cpptest-output.h"
#include "cpptest-source.h"
#include "cpptest-time.h"

namespace Test
{
	/// \brief Collector output.
	///
	/// Base class for output handlers that need to report status when all
	/// tests have executed.
	///
	class CollectorOutput : public Output
	{
	public:
		virtual void finished(int tests, const Time& time);
		virtual void suite_start(int tests, const std::string& name);
		virtual void suite_end(int tests, const std::string& name,
							   const Time& time);
		virtual void test_start(const std::string& name);
		virtual void test_end(const std::string& name, bool ok,
							  const Time& time);
		virtual void assertment(const Source& s);
		
	protected:
		struct OutputSuiteInfo;
		struct OutputTestInfo;
		struct OutputErrorTestInfo;
		
		typedef std::list<Source> Sources;
		
		struct TestInfo
		{
            std::string _name;
			Time		_time;
			
			bool		_success : 1;
			Sources		_sources;
			
			explicit TestInfo(const std::string name);
		};
		
		typedef std::vector<TestInfo> Tests;
		
		struct SuiteInfo
		{
			std::string	_name;
			int			_errors;
			Tests		_tests;
			Time		_time;
			
			SuiteInfo(const std::string& name, int tests);
		};
		
		typedef std::list<SuiteInfo> Suites;
		
		Suites			_suites;
		int				_total_errors;
		int				_total_tests;
		Time			_total_time;
		
		CollectorOutput();
		
	private:
		SuiteInfo*		_cur_suite;
		TestInfo*		_cur_test;
	};
	
} // namespace Test
	
#endif // #ifndef CPPTEST_COLLECTOROUTPUT_H

