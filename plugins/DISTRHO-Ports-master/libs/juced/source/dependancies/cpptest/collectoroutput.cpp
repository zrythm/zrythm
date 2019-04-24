// ---
//
// $Id: collectoroutput.cpp,v 1.4 2008/07/15 20:33:31 hartwork Exp $
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

#include "cpptest-collectoroutput.h"

namespace Test
{
	CollectorOutput::TestInfo::TestInfo(const std::string name)
	:	_name(name)
	{}

	CollectorOutput::SuiteInfo::SuiteInfo(const std::string& name, int tests)
	:	_name(name),
		_errors(0)
	{
		_tests.reserve(tests);
	}
	
	/// Constructs a collector object.
	///
	CollectorOutput::CollectorOutput()
	:	Output(),
		_total_errors(0)
	{}
	
	void
	CollectorOutput::finished(int tests, const Time& time)
	{
		_total_tests = tests;
		_total_time  = time;
	}
	
	void
	CollectorOutput::suite_start(int tests, const std::string& name)
	{
		if (tests > 0)
		{
			_suites.push_back(SuiteInfo(name, tests));
			_cur_suite = &_suites.back();
		}
	}
	
	void
	CollectorOutput::suite_end(int tests, const std::string&, const Time& time)
	{
		if (tests > 0)
		{
			_cur_suite->_time = time;
			_total_errors += _cur_suite->_errors;
		}
	}
	
	void
	CollectorOutput::test_start(const std::string& name)
	{
		_cur_suite->_tests.push_back(TestInfo(name));
		_cur_test = &_cur_suite->_tests.back();
	}
	
	void
	CollectorOutput::test_end(const std::string&, bool ok, const Time& time)
	{
		if (!(_cur_test->_success = ok))
			++_cur_suite->_errors;
		_cur_test->_time    = time;
	}
	
	void
	CollectorOutput::assertment(const Source& s)
	{
		_cur_test->_sources.push_back(s);
	}
	
} // namespace Test

