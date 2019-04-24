// ---
//
// $Id: cpptest-source.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
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

#ifndef CPPTEST_SOURCE_H
#define CPPTEST_SOURCE_H

namespace Test
{
	class Suite;
	
	/// \brief Assertment source information.
	///
	/// Contains information about an assertment point.
	///
	class Source
	{
		friend class Suite;
		
	public:
		Source();
		Source(const char* file, unsigned int line, const char* msg);
		
		const std::string& file() const;
		unsigned int line() const;
		const std::string& message() const;
		const std::string& suite() const;
		const std::string& test() const;
		
	private:
		unsigned int	_line;
		std::string		_file;
		std::string		_msg;
		std::string		_suite;
		std::string		_test;
	};
	
} // namespace Test

#endif // #ifndef CPPTEST_SOURCE_H

