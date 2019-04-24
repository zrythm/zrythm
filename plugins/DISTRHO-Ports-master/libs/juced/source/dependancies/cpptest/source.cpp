// ---
//
// $Id: source.cpp,v 1.4 2005/06/08 09:25:09 nilu Exp $
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

#include "cpptest-source.h"

using namespace std;

namespace Test
{
	/// Constructs an invalid source object, which filename and message are
	/// empty strings and the line equals zero.
	///
	Source::Source()
	:	_line(0)
	{}
	
	/// Constructs a source object.
	///
	/// \param file Name of the file containing the failing function.
	/// \param line Line where the function starts.
	/// \param msg  Expression (or message) that caused the failure.
	///
	Source::Source(const char* file, unsigned int line, const char* msg)
	:	_line(line),
		_file(file ? file : ""),
		_msg(msg ? msg : "")
	{}
	
	/// \return Name of the file containing the failing function.
	///
	const string&
	Source::file() const 	
	{
		return _file;
	}
	
	/// \return Line where the function starts.
	///
	unsigned int
	Source::line() const
	{
		return _line;
	}
	
	/// \return Descriptive message.
	///
	const string&
	Source::message() const
	{
		return _msg;
	}
	
	/// \return Name of the suite, which the test belongs to.
	///
	const string&
	Source::suite() const
	{
		return _suite;
	}

	/// \return Name of failing test.
	///
	const string&
	Source::test() const
	{
		return _test;
	}

} // namespace Test

