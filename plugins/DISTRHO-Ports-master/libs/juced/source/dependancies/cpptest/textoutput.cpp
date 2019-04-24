// ---
//
// $Id: textoutput.cpp,v 1.4 2008/07/15 20:33:31 hartwork Exp $
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

#include "cpptest-textoutput.h"
#include "cpptest-time.h"
#include "utils.h"

namespace Test
{
	namespace
	{
		// Outputs detailed assert source information. Used in verbose mode.
		//
		struct ShowSource
		{
            std::ostream& _stream;
			ShowSource(std::ostream& stream) : _stream(stream) {}
			void operator()(const Source& s)
			{
				_stream << "\tTest:    " << s.test()    << std::endl
						<< "\tSuite:   " << s.suite()   << std::endl
						<< "\tFile:    " << s.file()    << std::endl
						<< "\tLine:    " << s.line()    << std::endl
						<< "\tMessage: " << s.message() << std::endl << std::endl;
						
			}
		};
		
	} // anonymous namespace
	
	/// Constructs a text output handler.
	///
	/// \param mode   Output mode.
	/// \param stream Stream to output to.
	///
	TextOutput::TextOutput(Mode mode, std::ostream& stream)
	:   _mode(mode),
		_stream(stream),
		_total_errors(0)
	{}
	
	void
	TextOutput::finished(int tests, const Time& time)
	{
		_stream	<< "Total: " << tests << " tests, "
				<< correct(tests, _total_errors) << "% correct"
				<< " in " << time << " seconds" << std::endl;
	}
	
	void
	TextOutput::suite_start(int tests, const std::string& name)
	{
		if (tests > 0)
		{
			_suite_name  = name;
			_suite_tests = _suite_errors = 0;
			_suite_total_tests = tests;
			_suite_error_list.clear();
			
			_stream	<< _suite_name << ": "
					<< "0/" << _suite_total_tests
					<< "\r" << std::flush;
		}
	}
	
	void
	TextOutput::suite_end(int tests, const std::string& name, const Time& time)
	{
		if (tests > 0)
		{
			_stream	<< name << ": " << tests << "/" << tests << ", "
					<< correct(tests, _suite_errors) << "% correct"
					<< " in " << time << " seconds" << std::endl;
            			
			if (_mode == Verbose && _suite_errors)
				std::for_each(_suite_error_list.begin(), _suite_error_list.end(),
						      ShowSource(_stream));
			
			_total_errors += _suite_errors;
		}
	}
	
	void
	TextOutput::test_end(const std::string&, bool ok, const Time&)
	{
		_stream	<< _suite_name << ": "
				<< ++_suite_tests << "/" << _suite_total_tests
				<< "\r" << std::flush;
		if (!ok)
			++_suite_errors;
	}
	
	void
	TextOutput::assertment(const Source& s)
	{
		_suite_error_list.push_back(s);
	}
	
} // namespace Test
