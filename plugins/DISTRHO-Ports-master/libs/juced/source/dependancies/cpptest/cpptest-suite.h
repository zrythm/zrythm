// ---
//
// $Id: cpptest-suite.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
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

#ifndef CPPTEST_SUITE_H
#define CPPTEST_SUITE_H

#include "cpptest-time.h"

namespace Test
{
	class Output;
	
	/// \brief Unit testing suite.
	///
	/// Base class for all suites. Derive from this class to create own
	/// test suites.
	///
	/// %Test functions in derived classes are registered as tests using the
	/// TEST_ADD(func). Testing is started by run(). Note that suites may be
	/// embedded in other suites using add().
	///
	class Suite
	{
	public:	
		Suite();
		virtual ~Suite();
		
		void add(Suite* suite);
		
		bool run(Output& output, bool cont_after_fail = true);
		
	protected:
		/// Pointer to a test function.
		///
		typedef void (Suite::*Func)();
		
		bool continue_after_failure() const { return _continue; }
		
		virtual void setup()     {}
		virtual void tear_down() {}
		
		void register_test(Func func, const std::string& name);
		void assertment(Source s);
		
	private:
		struct DoRun;
		struct ExecTests;
		struct SubSuiteTests;
		struct SuiteTime;
		struct SubSuiteTime;
		
		friend struct DoRun;
		friend struct ExecTests;
		friend struct SubSuiteTests;
		friend struct SubSuiteTime;
		
		struct Data
		{
			Func			_func;
			std::string		_name;
			Time			_time;
		
			Data(Func func, const std::string& name)
				: _func(func), _name(name) {}
		};
		
		typedef std::list<Data> 	Tests;
		typedef std::list<Suite*> 	Suites;
		
		std::string			_name;			// Suite name
		const std::string*	_cur_test;		// Current test func name
        Suites				_suites;		// External test suites
		Tests 				_tests;			// All tests
		Output*				_output;		// Output handler
		bool				_result   : 1;	// Test result
		bool				_success  : 1;	// Set if no test failed
		bool				_continue : 1;	// Continue func after failures
		
		void do_run(Output* os, bool cont_after_fail);
		int total_tests() const;
		Time total_time(bool recursive) const;
		
		// Disable
		//
		Suite(const Suite&);
		Suite& operator=(const Suite&);
	};

	/// Adds a test function to the enclosing suite. Note that test functions
	/// should be added in the suites constructor.
	///
	/// \param func Function to add, must be of type Suite::Func.
	///
	/// \par Example:
	/// \code
	///	MySuite::MySuite()
	///	{
	/// 	TEST_ADD(&MySuite::test_1)
	/// 	TEST_ADD(&MySuite::test_2)
	/// 	...
	///	}
	/// \endcode
	///
	#define TEST_ADD(func)\
		register_test(static_cast<Func>(&func), #func);
	
} // namespace Test

#endif // #ifndef CPPTEST_SUITE_H

