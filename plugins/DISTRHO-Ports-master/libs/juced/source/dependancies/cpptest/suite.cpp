// ---
//
// $Id: suite.cpp,v 1.6 2008/07/15 20:33:31 hartwork Exp $
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

#include "cpptest-output.h"
#include "cpptest-source.h"
#include "cpptest-suite.h"

namespace Test
{
	namespace
	{
		// Destroys all dynamically allocated objects within the given range.
		//
		template <class FwdIter>
		void
		destroy_range(FwdIter first, FwdIter last)
		{
			while (first != last)
				delete *first++;
		}
		
	} // anonymous namespace
	
	/// Constructs an empty test suite.
	///
	Suite::Suite()
	:	_cur_test(0),
		_output(0),
		_success(true)
	{}
	
	/// Destroys this suite object.
	///
	Suite::~Suite()
	{
		destroy_range(_suites.begin(), _suites.end());
	}
	
	/// Starts the testing. All tests in this suite and embedded suites will
	/// be executed.
	///
	/// \param output          Progress report destination.
	/// \param cont_after_fail Continue functions despite failures.
	///
	/// \return True if no test failed; false otherwise.
	///
	bool
	Suite::run(Output& output, bool cont_after_fail)
	{
		int ntests = total_tests();
		output.initialize(ntests);
		do_run(&output, cont_after_fail);
		output.finished(ntests, total_time(true));
		return _success;
	}
		
	/// \fn void Suite::setup()
	///
	/// Setups a test fixture. This function is called before each test,
	/// in this suite, is executed.
	///
	/// This function should be overloaded by derived classes to provide
	/// specialized behavior.
	///
	/// \see tear_down()
	
	/// \fn void Suite::tear_down()
	///
	/// Tears down a test fixture. This function is called after each test,
	/// in this suite, have been executed.
	///
	/// This function should be overloaded by derived classes to provide
	/// specialized behavior.
	///
	/// \see setup()
	
	/// Adds a suite to this suite. Tests in added suites will be executed
	/// when run() of the top-level suite is called.
	///
	/// \param suite %Test suite to add.
	///
	void
	Suite::add(Suite* suite)
	{
		_suites.push_back(suite);
	}
	
	/// Registers a test function.
	///
	/// \b Note: Do not call this function directly, use the TEST_ADD(func)
	/// macro instead.
	///
	/// \param func Pointer to a test function.
	/// \param name Class and function name of the function. The format \b must
	///             equal \e class::func.
	///
	void
	Suite::register_test(Func func, const std::string& name)
	{
		string::size_type pos = name.find_first_of(':');
		//assert(!name.empty() && name[pos + 1] == ':' && name[pos + 2] != '\0');
		
		_name.assign(name, 0, pos);
		_tests.push_back(Data(func, name.substr(pos + 2)));
	}
	
	/// Issues an assertment to the output handler.
	///
	/// Do not call this function directly, use one of the available assertment
	/// macros instead, see \ref asserts.
	///
	/// \param s Assert point information.
	///
	void
	Suite::assertment(Source s)
	{
		s._suite = _name;
		s._test  = *_cur_test;
		_output->assertment(s);
		_result = _success = false;
	}
	
	// Functor to execute tests for the given suite.
	//
	struct Suite::ExecTests
	{
		Suite& _suite;
		
		ExecTests(Suite& s) : _suite(s) {}
		
		void operator()(Data& data)
		{
			_suite._cur_test = &data._name;
			_suite._result = true; // assume success, assert will set to false
			_suite._output->test_start(data._name);
			
			_suite.setup();
			Time start(Time::current());
			// FIXME Also feedback exception to user
			try
			{
				(_suite.*data._func)();
			} catch (...) {
				_suite._result = false;
			}
			Time end(Time::current());
			_suite.tear_down();
			
			data._time = end - start;
			_suite._output->test_end(data._name, _suite._result, data._time);
		}
	};

	// Functor to execute a suite.
	//
	struct Suite::DoRun
	{
		bool	_continue;
		Output* _output;
		
		DoRun(Output* output, bool cont) : _continue(cont), _output(output) {}
		void operator()(Suite* suite) { suite->do_run(_output, _continue); }
	};

	// Execute all tests in this and added suites.
	//
	void
	Suite::do_run(Output* os, bool cont_after_fail)
	{
		_continue = cont_after_fail;
		_output = os;
		
		_output->suite_start(_tests.size(), _name);
		std::for_each(_tests.begin(), _tests.end(), ExecTests(*this));
		_output->suite_end(_tests.size(), _name, total_time(false));

		std::for_each(_suites.begin(), _suites.end(), DoRun(_output, _continue));

		// FIXME Find a cleaner way
		Suites::const_iterator iter = _suites.begin();
		while (iter != _suites.end())
		{
			if (!(*iter)->_success)
			{
				_success = false;
				break;
			}
			iter++;
		}
	}

	// Functor to count all tests in a suite.
	//
	struct Suite::SubSuiteTests
	{
		int operator()(size_t value, const Suite* s) const
		{
			return value + s->total_tests();
		}
	};
	
	// Counts all tests in this and all its embedded suites.
	//
	int
	Suite::total_tests() const
	{
		return accumulate(_suites.begin(), _suites.end(),
						  _tests.size(), SubSuiteTests());
	}
	
	// Functor to accumulate execution time for tests.
	//
	struct Suite::SuiteTime
	{
		Time operator()(const Time& time, const Data& data)
		{
			return time + data._time;
		}
	};
	
	// Functor to accumulate execution time for suites.
	//
	struct Suite::SubSuiteTime
	{
		Time operator()(Time time, const Suite* s) const
		{
			return time + s->total_time(true);
		}
	};
	
	// Counts time accumulated execution time for all tests in this and all
	// its embedded suites.
	//
	Time
	Suite::total_time(bool recursive) const
	{
		Time time = accumulate(_tests.begin(), _tests.end(),
							   Time(), SuiteTime());
		
		return !recursive ? time : accumulate(_suites.begin(), _suites.end(),
											  time, SubSuiteTime());
	}
	
} // namespace Test
